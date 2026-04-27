module;

#include <cassert>
#include <concepts>
#include <cstddef>
#include <vector>

export module dag;

using NodeID = std::size_t;
using InputID = std::size_t;
using OutputID = std::size_t;

// Range of [begin, end)
export template <std::equality_comparable Index> struct IndexRange {
  Index begin;
  Index end;

  IndexRange(Index begin, Index end) : begin(begin), end(end) {
    // PRECONDITION
    assert(begin <= end && "The beginning of a region must be before the end.");
  }
};

export template <typename T, typename Type> struct Port {
  T data;
  Type type;
};


export class BitAdjMatrix {
public:
  BitAdjMatrix() = default;

  auto operator()(this const auto& self, size_t row, size_t column) -> bool {
    return self.data[row][column];
  }
  
  auto add(this auto& self, size_t row, size_t column) -> void {
    if (row >= self.data.size()) {
      self.data.resize(row + 1);
    }
    
    if (column >= self.data[0].size()) {
      for (auto &c : self.data) {
        c.resize(column + 1);
      }
    }

    self.data[row][column] = true;
  }
private:
  std::vector<std::vector<bool>> data;
};

export template <typename N, typename P, typename Type> class DAG {
public:
  auto add_node(this auto &self, N node,
                const std::vector<Port<P, Type>> &inputs,
                const std::vector<Port<P, Type>> &outputs) -> NodeID {
    self.nodes.push_back(node);
    const NodeID nodeIndex = static_cast<NodeID>(self.nodes.size() - 1);

    self.inputs.insert(self.inputs.end(), inputs.begin(), inputs.end());
    self.outputs.insert(self.outputs.end(), outputs.begin(), outputs.end());

    const IndexRange<InputID> inputRange = IndexRange<InputID>(
        self.inputs.size() - inputs.size(),
        self.inputs.size()
    );
    const IndexRange<InputID> outputRange = IndexRange<InputID>(
        self.outputs.size() - outputs.size(),
        self.outputs.size()
    );

    self.nodeInputIndicies.push_back(inputRange);
    self.nodeOutputIndicies.push_back(outputRange);

    // POSTCONDITIONS
    assert(self.nodes.size() == self.nodeInputIndicies.size() &&
           self.nodeInputIndicies.size() == self.nodeOutputIndicies.size());

    return nodeIndex;
  }
  auto add_edge(this auto& self, OutputID from, InputID to) -> void {
    self.edges.add(from, to);
  }

private:
  template <typename T> using vec = std::vector<T>;
  // Indexable via [NodeID]
  vec<N> nodes;
  vec<IndexRange<InputID>> nodeInputIndicies;
  vec<IndexRange<OutputID>> nodeOutputIndicies;

  // Indexable via [InputID]
  vec<Port<P, Type>> inputs;

  // Indexable via [OutputID]
  vec<Port<P, Type>> outputs;
  

  BitAdjMatrix edges;
};
