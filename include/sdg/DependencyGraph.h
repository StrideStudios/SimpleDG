#pragma once

#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <queue>

template <typename TType, typename TTopologicalSorter>
struct TDependencyGraph {

    virtual ~TDependencyGraph() = default;

    TType& getNode(size_t id) { return nodes[id]; }
    const TType& getNode(size_t id) const { return nodes[id]; }

    template <typename... TArgs>
    size_t addNode(TArgs&&... args) {
        const size_t nodeId = nodes.size();
        nodes.emplace_back(std::forward<TArgs>(args)...);
        return nodeId;
    }

    virtual std::vector<size_t> buildExecutionOrder() = 0;

protected:

    std::vector<TType> nodes;
    TTopologicalSorter sorter;
};

// Has simple dependencies
template <typename TType, typename TTopologicalSorter>
struct TSimpleDependencyGraph : TDependencyGraph<TType, TTopologicalSorter> {

    using TDependencyGraph<TType, TTopologicalSorter>::nodes;
    using TDependencyGraph<TType, TTopologicalSorter>::sorter;

    void addDependency(const size_t node, const size_t dependency) {
        dependencies[node].push_back(dependency);
    }

    virtual std::vector<size_t> buildExecutionOrder() override {
        return sorter(nodes, dependencies);
    }

private:

    std::unordered_map<size_t, std::vector<size_t>> dependencies;

};

// Read and Write dependencies
template <typename TType, typename TDependencyType, typename TTopologicalSorter>
struct TRWDependencyGraph : TDependencyGraph<TType, TTopologicalSorter> {

    struct Access {
        TDependencyType node;
        enum { READ, WRITE } type;
    };

    using TDependencyGraph<TType, TTopologicalSorter>::nodes;
    using TDependencyGraph<TType, TTopologicalSorter>::sorter;

    void addRead(size_t node, const TDependencyType dependency) {
        dependencies[node].emplace_back(Access{dependency, Access::READ});
    }

    void addWrite(size_t node, const TDependencyType dependency) {
        dependencies[node].emplace_back(Access{dependency, Access::WRITE});
    }

    virtual std::vector<size_t> buildExecutionOrder() override {
        std::unordered_map<size_t, std::vector<size_t>> outDependencies;

        struct ResourceState {
            size_t lastWriter = SIZE_MAX;
           std::unordered_set<size_t> lastReaders;
        };

        struct Hasher {
            size_t operator()(const TDependencyType& p) const noexcept {
                return getHash(p);
            }
        };

        std::unordered_map<TDependencyType, ResourceState, Hasher> resourceStates;

        for (const auto& [node, accesses] : dependencies) {
            for (const auto& access : accesses) {
                ResourceState& currentResourceState = resourceStates[access.node];

                switch (access.type) {
                case Access::READ:
                    // RAW - When reading from a resource, the last one who wrote to it must run first
                    if (currentResourceState.lastWriter != SIZE_MAX && currentResourceState.lastWriter != node)
                        outDependencies[currentResourceState.lastWriter].emplace_back(node);
                    currentResourceState.lastReaders.emplace(node);
                    break;
                case Access::WRITE:
                    // WAW - When writing to a resource, we must wait on the previous writer before writing to it
                    if (currentResourceState.lastWriter != SIZE_MAX && currentResourceState.lastWriter != node)
                        outDependencies[currentResourceState.lastWriter].emplace_back(node);
                    // WAR - When writing to a resource, we must wait on the previous readers before writing to it, as to not change it while reading
                    for (size_t reader : currentResourceState.lastReaders)
                        if (reader != node)
                            outDependencies[reader].emplace_back(node);
                    currentResourceState.lastReaders.clear();
                    currentResourceState.lastWriter = node;
                    break;
                default: break;
                }
            }
        }

        return sorter(nodes, outDependencies);
    }

    std::unordered_map<size_t, std::vector<Access>> dependencies;
};


// Great for simple graphs, but dependents are not always after their base, even if there are no other dependents
// Essentially uses a brute force approach, calculating dependents one by one, despite this, it is quite fast and space efficient
struct TKahnTopologicalSort {

    template <typename TType>
    std::vector<size_t> operator()(const std::vector<TType>& nodes, const std::unordered_map<size_t, std::vector<size_t>>& dependencies) const {
        std::vector<int> inDegree;

        // Each node starts with 0 dependencies
        for (size_t i = 0; i < nodes.size(); ++i)
            inDegree.push_back(0);

        // Add one whenever a node is a dependency, a node that nothing depends on will be 0
        for (size_t i = 0; i < nodes.size(); ++i)
            if (dependencies.find(i) != dependencies.end())
                for (size_t to : dependencies.at(i))
                    ++inDegree[to];

        // Each node that nothing depends on will be added to queue
        std::queue<size_t> q;
        for (size_t id = 0; id < inDegree.size(); ++id)
            if (inDegree[id] == 0)
                q.push(id);

        std::vector<size_t> order;

        while (!q.empty()) {

            // Push the nodes with no dependencies into the list
            size_t id = q.front(); q.pop();
            order.push_back(id);

            // For each node that is no longer a dependent, add to the queue
            if (dependencies.find(id) != dependencies.end()) {
                for (auto& dependency : dependencies.at(id)) {
                    if (--inDegree[dependency] == 0) {
                        q.push(dependency);
                    }
                }
            }
        }

        if (order.size() != nodes.size())
            throw std::runtime_error("Cycle detected in dependency graph!");

        return order;
    }
};