// Project identifier: 43DE0E0C4C76BFAA6D8C2F5AEAE0518A9C261F4E

#ifndef PAIRINGPQ_H
#define PAIRINGPQ_H

#include <deque>
#include <utility>

#include "Eecs281PQ.hpp"

// A specialized version of the priority queue ADT implemented as a pairing
// heap.
template<typename TYPE, typename COMP_FUNCTOR = std::less<TYPE>>
class PairingPQ : public Eecs281PQ<TYPE, COMP_FUNCTOR> {
    // This is a way to refer to the base class object.
    using BaseClass = Eecs281PQ<TYPE, COMP_FUNCTOR>;

public:
    // Each node within the pairing heap
    class Node {
    public:
        // Description: Custom constructor that creates a node containing
        //              the given value.
        explicit Node(const TYPE &val)
            : elt { val } {}

        // Description: Allows access to the element at that Node's position.
        //              There are two versions, getElt() and a dereference
        //              operator, use whichever one seems more natural to you.
        // Runtime: O(1) - this has been provided for you.
        const TYPE &getElt() const { return elt; }
        const TYPE &operator*() const { return elt; }

        // The following line allows you to access any private data
        // members of this Node class from within the PairingPQ class.
        // (ie: myNode.elt is a legal statement in PairingPQ's add_node()
        // function).
        friend PairingPQ;

    private:
        TYPE elt;
        Node *child = nullptr;
        Node *sibling = nullptr;
        // Points to the parent if this is the leftmost child, otherwise to
        // the sibling on the left. Null for the root.
        Node *previous = nullptr;
    };  // Node


    // Description: Construct an empty pairing heap with an optional
    //              comparison functor.
    // Runtime: O(1)
    explicit PairingPQ(COMP_FUNCTOR comp = COMP_FUNCTOR())
        : BaseClass { comp } {}  // PairingPQ()


    // Description: Construct a pairing heap out of an iterator range with an
    //              optional comparison functor.
    // Runtime: O(n) where n is number of elements in range.
    template<typename InputIterator>
    PairingPQ(InputIterator start, InputIterator end, COMP_FUNCTOR comp = COMP_FUNCTOR())
        : BaseClass { comp } {
        for (; start != end; ++start) {
            push(*start);
        }  // for
    }  // PairingPQ()


    // Description: Copy constructor.
    // Runtime: O(n)
    PairingPQ(const PairingPQ &other)
        : BaseClass { other.compare } {
        if (!other.root) {
            return;
        }  // if
        std::deque<Node *> nodes { other.root };
        while (!nodes.empty()) {
            Node *cur = nodes.front();
            nodes.pop_front();
            if (cur->child) {
                nodes.push_back(cur->child);
            }  // if
            if (cur->sibling) {
                nodes.push_back(cur->sibling);
            }  // if
            push(cur->elt);
        }  // while
    }  // PairingPQ()


    // Description: Copy assignment operator.
    // Runtime: O(n)
    PairingPQ &operator=(const PairingPQ &rhs) {
        PairingPQ temp { rhs };
        std::swap(root, temp.root);
        std::swap(count, temp.count);
        return *this;
    }  // operator=()


    // Description: Destructor
    // Runtime: O(n)
    ~PairingPQ() {
        if (!root) {
            return;
        }  // if
        std::deque<Node *> nodes { root };
        while (!nodes.empty()) {
            Node *cur = nodes.front();
            nodes.pop_front();
            if (cur->child) {
                nodes.push_back(cur->child);
            }  // if
            if (cur->sibling) {
                nodes.push_back(cur->sibling);
            }  // if
            delete cur;
        }  // while
    }  // ~PairingPQ()


    // Description: Move constructor and assignment operator. These steal the
    //              other heap's nodes; a defaulted version would copy the
    //              raw root pointer and both destructors would free it.
    PairingPQ(PairingPQ &&other) noexcept
        : BaseClass { std::move(other) }
        , root { other.root }
        , count { other.count } {
        other.root = nullptr;
        other.count = 0;
    }  // PairingPQ()

    PairingPQ &operator=(PairingPQ &&rhs) noexcept {
        std::swap(root, rhs.root);
        std::swap(count, rhs.count);
        return *this;
    }  // operator=()


    // Description: Assumes that all elements inside the pairing heap are out
    //              of order and 'rebuilds' the pairing heap by fixing the
    //              pairing heap invariant.  You CANNOT delete 'old' nodes
    //              and create new ones!
    // Runtime: O(n)
    virtual void updatePriorities() {
        if (!root) {
            return;
        }  // if
        // Detach every node into a standalone heap, then meld them back in.
        std::deque<Node *> nodes { root };
        root = nullptr;
        while (!nodes.empty()) {
            Node *cur = nodes.front();
            nodes.pop_front();
            if (cur->child) {
                nodes.push_back(cur->child);
            }  // if
            if (cur->sibling) {
                nodes.push_back(cur->sibling);
            }  // if
            cur->child = nullptr;
            cur->sibling = nullptr;
            cur->previous = nullptr;
            root = root ? meld(root, cur) : cur;
        }  // while
    }  // updatePriorities()


    // Description: Add a new element to the pairing heap. This is already
    //              done. You should implement push functionality entirely
    //              in the addNode() function, and this function calls
    //              addNode().
    // Runtime: O(1)
    virtual void push(const TYPE &val) { addNode(val); }  // push()


    // Description: Remove the most extreme (defined by 'compare') element
    //              from the pairing heap.
    // Note: We will not run tests on your code that would require it to pop
    // an element when the pairing heap is empty. Though you are welcome to
    // if you are familiar with them, you do not need to use exceptions in
    // this project.
    // Runtime: Amortized O(log(n))
    virtual void pop() {
        Node *first = root->child;
        delete root;
        root = nullptr;
        --count;

        // Multi-pass: meld the old root's children two at a time, front to
        // back, until a single heap remains.
        std::deque<Node *> heaps;
        while (first) {
            Node *next = first->sibling;
            first->sibling = nullptr;
            first->previous = nullptr;
            heaps.push_back(first);
            first = next;
        }  // while
        while (heaps.size() > 1) {
            Node *a = heaps.front();
            heaps.pop_front();
            Node *b = heaps.front();
            heaps.pop_front();
            heaps.push_back(meld(a, b));
        }  // while
        if (!heaps.empty()) {
            root = heaps.front();
        }  // if
    }  // pop()


    // Description: Return the most extreme (defined by 'compare') element of
    //              the pairing heap. This should be a reference for speed.
    //              It MUST be const because we cannot allow it to be
    //              modified, as that might make it no longer be the most
    //              extreme element.
    // Runtime: O(1)
    virtual const TYPE &top() const { return root->elt; }  // top()


    // Description: Get the number of elements in the pairing heap.
    // Runtime: O(1)
    [[nodiscard]] virtual std::size_t size() const { return count; }  // size()

    // Description: Return true if the pairing heap is empty.
    // Runtime: O(1)
    [[nodiscard]] virtual bool empty() const { return count == 0; }  // empty()


    // Description: Updates the priority of an element already in the pairing
    //              heap by replacing the element refered to by the Node with
    //              new_value.  Must maintain pairing heap invariants.
    //
    // PRECONDITION: The new priority, given by 'new_value' must be more
    //              extreme (as defined by comp) than the old priority.
    //
    // Runtime: As discussed in reading material.
    void updateElt(Node *node, const TYPE &new_value) {
        node->elt = new_value;
        if (node == root) {
            return;
        }  // if

        // Cut the node (with its subtree) out of its sibling list.
        if (node->previous->child == node) {
            node->previous->child = node->sibling;
        } else {
            node->previous->sibling = node->sibling;
        }  // if
        if (node->sibling) {
            node->sibling->previous = node->previous;
        }  // if
        node->sibling = nullptr;
        node->previous = nullptr;

        root = meld(root, node);
    }  // updateElt()


    // Description: Add a new element to the pairing heap. Returns a Node*
    //              corresponding to the newly added element.
    // Runtime: O(1)
    // NOTE: Whenever you create a node, and thus return a Node *, you must
    //       be sure to never move or copy/delete that node in the future,
    //       until it is eliminated by the user calling pop(). Remember this
    //       when you implement updateElt() and updatePriorities().
    Node *addNode(const TYPE &val) {
        Node *node = new Node { val };
        root = root ? meld(root, node) : node;
        ++count;
        return node;
    }  // addNode()


private:
    Node *root = nullptr;
    std::size_t count = 0;

    // Description: Combine two heaps whose roots have no siblings and no
    //              previous pointer. The less extreme root becomes the
    //              leftmost child of the other. Returns the new root.
    // Runtime: O(1)
    Node *meld(Node *a, Node *b) {
        if (this->compare(a->elt, b->elt)) {
            std::swap(a, b);
        }  // if
        b->sibling = a->child;
        if (a->child) {
            a->child->previous = b;
        }  // if
        b->previous = a;
        a->child = b;
        return a;
    }  // meld()
};


#endif  // PAIRINGPQ_H
