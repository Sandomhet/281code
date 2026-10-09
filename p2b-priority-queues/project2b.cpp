// Project identifier: 43DE0E0C4C76BFAA6D8C2F5AEAE0518A9C261F4E

/*
 * Compile this test against your .h files to make sure they compile. We
 * suggest adding to this file or creating your own test cases to test your
 * priority queue implementations more thoroughly. If you do not call a
 * function from here, that template is not instantiated and that function is
 * NOT compiled! So for instance, if you don't add code here to call
 * updatePriorities, that function could later cause compiler errors that you
 * don't even know about.
 *
 * Our makefile will build an executable named testPQ if you type 'make
 * testPQ' or 'make alltests' (without the quotes). This will be a debug
 * executable.
 *
 * Notice that testPairing tests the range-based constructor but main and
 * testPriorityQueue do not. Make sure to test the range-based constructor
 * for other PQ types, and also test the PairingPQ-specific member functions.
 *
 * This is NOT a complete test of your priority queues. You have to add code
 * to do more testing!
 *
 * You do not have to submit this file, but it won't cause problems if you
 * do.
 */

#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "BinaryPQ.hpp"
#include "Eecs281PQ.hpp"
#include "PairingPQ.hpp"
#include "SortedPQ.hpp"
#include "UnorderedPQ.hpp"

// A type for representing priority queue types at runtime
enum class PQType {
    Unordered,
    Sorted,
    Binary,
    Pairing,
};

// These can be pretty-printed :)
std::ostream &operator<<(std::ostream &ost, PQType pqType) {
    switch (pqType) {
    case PQType::Unordered:
        return ost << "Unordered";
    case PQType::Sorted:
        return ost << "Sorted";
    case PQType::Binary:
        return ost << "Binary";
    case PQType::Pairing:
        return ost << "Pairing";
    } // switch

    return ost << "Unknown PQType";
} // operator<<()


// Compares two int const* on the integers they point to
struct IntPtrComp {
    bool operator()(const int *a, const int *b) const {
        return *a < *b;
    } // operator()
}; // IntPtrComp structure


// Test the primitive operations on a priority queue:
// constructor, push, pop, top, size, empty.
template <template <typename...> typename PQ>
void testPrimitiveOperations() {
    std::cout << "Testing primitive priority queue operations..." << std::endl;

    PQ<int> pq {};
    Eecs281PQ<int> &eecsPQ = pq;

    eecsPQ.push(3);
    eecsPQ.push(4);
    assert(eecsPQ.size() == 2);
    assert(eecsPQ.top() == 4);

    eecsPQ.pop();
    assert(eecsPQ.size() == 1);
    assert(eecsPQ.top() == 3);
    assert(not eecsPQ.empty());

    eecsPQ.pop();
    assert(eecsPQ.size() == 0);  // NOLINT: Explicit test for size == 0
    assert(eecsPQ.empty());

    // Duplicates and interleaved push/pop
    for (int v : { 5, 1, 5, 9, 3, 9, 0 }) {
        eecsPQ.push(v);
    } // for
    assert(eecsPQ.size() == 7);
    assert(eecsPQ.top() == 9);
    eecsPQ.pop();
    assert(eecsPQ.top() == 9);
    eecsPQ.pop();
    eecsPQ.push(7);
    assert(eecsPQ.top() == 7);
    for (int expected : { 7, 5, 5, 3, 1, 0 }) {
        assert(eecsPQ.top() == expected);
        eecsPQ.pop();
    } // for
    assert(eecsPQ.empty());

    // Single element
    eecsPQ.push(42);
    assert(eecsPQ.top() == 42);
    eecsPQ.pop();
    assert(eecsPQ.empty());

    // Range constructor
    const std::vector<int> vec { 4, 8, 1, 8, 6, 2 };
    PQ<int> ranged { vec.begin(), vec.end() };
    assert(ranged.size() == vec.size());
    for (int expected : { 8, 8, 6, 4, 2, 1 }) {
        assert(ranged.top() == expected);
        ranged.pop();
    } // for
    assert(ranged.empty());

    // Min-PQ via std::greater
    PQ<int, std::greater<int>> minPQ { vec.begin(), vec.end() };
    for (int expected : { 1, 2, 4, 6, 8, 8 }) {
        assert(minPQ.top() == expected);
        minPQ.pop();
    } // for
    assert(minPQ.empty());

    // Empty range constructor
    PQ<int> emptyRanged { vec.begin(), vec.begin() };
    assert(emptyRanged.empty());
    emptyRanged.push(1);
    assert(emptyRanged.top() == 1);

    std::cout << "testPrimitiveOperations succeeded!" << std::endl;
} // testPrimitiveOperations()


// Test that the priority queue uses its comparator properly. HiddenData
// can't be compared with operator<, so we use HiddenDataComp{} instead.
template <template <typename...> typename PQ>
void testHiddenData() {
    struct HiddenData {
        int data;
    }; // HiddenData structure

    struct HiddenDataComp {
        bool operator()(const HiddenData &a, const HiddenData &b) const {
            return a.data < b.data;
        } // operator()()
    }; // comparator

    std::cout << "Testing with hidden data..." << std::endl;

    PQ<HiddenData, HiddenDataComp> pq {};
    Eecs281PQ<HiddenData, HiddenDataComp> &eecsPQ = pq;

    for (int v : { 3, 7, 1, 7, 4 }) {
        eecsPQ.push(HiddenData { v });
    } // for
    assert(eecsPQ.size() == 5);
    for (int expected : { 7, 7, 4, 3, 1 }) {
        assert(eecsPQ.top().data == expected);
        eecsPQ.pop();
    } // for
    assert(eecsPQ.empty());

    const std::vector<HiddenData> vec { { 2 }, { 9 }, { 5 } };
    PQ<HiddenData, HiddenDataComp> ranged { vec.begin(), vec.end() };
    assert(ranged.top().data == 9);

    std::cout << "testHiddenData succeeded!" << std::endl;
} // testHiddenData()


// Test the last public member function of Eecs281PQ, updatePriorities
template <template <typename...> typename PQ>
void testUpdatePriorities() {
    std::vector<int> data {
        1,
        5,  // NOLINT: Some non-trivial number needed here
    };

    PQ<const int *, IntPtrComp> pq {};
    Eecs281PQ<const int *, IntPtrComp> &eecsPQ = pq;

    // NOTE: If you add more data to the vector, don't push the pointers
    //   until AFTER the vector stops changing size! Think about why.
    for (auto &datum : data) {
        eecsPQ.push(&datum);
    } // for

    // Change some element in data (which is pointed to by an element in pq).
    // This new value should be higher than any other so its address will
    // wind qt the top adter updatePriorities.
    auto &datum = data[0];
    datum = 10;  // NOLINT: Some non-trivial number needed here
    eecsPQ.updatePriorities();
    assert(*eecsPQ.top() == 10);
    assert(eecsPQ.top() == &datum);

    // Larger scramble: reverse every priority.
    std::vector<int> many(50);  // NOLINT
    for (size_t i = 0; i < many.size(); ++i) {
        many[i] = static_cast<int>(i);
    } // for
    PQ<const int *, IntPtrComp> pq2 {};
    for (auto &v : many) {
        pq2.push(&v);
    } // for
    assert(*pq2.top() == 49);
    for (auto &v : many) {
        v = -v;
    } // for
    pq2.updatePriorities();
    for (int expected = 0; expected > -50; --expected) {
        assert(*pq2.top() == expected);
        pq2.pop();
    } // for
    assert(pq2.empty());

    std::cout << "testUpdatePriorities succeeded!" << std::endl;
} // testUpdatePriorities()


// Random interleaved operations, checked against UnorderedPQ.
template <template <typename...> typename PQ, typename COMP>
void testRandomAgainstUnordered() {
    std::srand(281);  // NOLINT
    PQ<int, COMP> pq {};
    UnorderedPQ<int, COMP> ref {};
    for (int i = 0; i < 20000; ++i) {  // NOLINT
        if (ref.empty() || std::rand() % 3 != 0) {  // NOLINT
            int v = std::rand() % 1000;  // NOLINT
            pq.push(v);
            ref.push(v);
        } else {
            assert(pq.top() == ref.top());
            pq.pop();
            ref.pop();
        } // if
        assert(pq.size() == ref.size());
    } // for
    while (!ref.empty()) {
        assert(pq.top() == ref.top());
        pq.pop();
        ref.pop();
    } // while
    assert(pq.empty());
} // testRandomAgainstUnordered()


// Test the pairing heap's range-based constructor, copy constructor,
// copy-assignment operator, and destructor
// TODO: Test other operations specific to this PQ type.
void testPairing() {
    std::cout << "Testing Pairing Heap separately..." << std::endl;

    // Start a block of code here, so that when it ends, we have
    // a way to know that the destructors are about to be called,
    // but the function hasn't finished yet.
    {
        const std::vector<int> vec { 1, 0, };

        std::cout << "Calling constructors" << std::endl;

        // Range-based constructor
        PairingPQ<int> pairing1 { vec.cbegin(), vec.cend() };

        // Copy constructor
        PairingPQ<int> pairing2 { pairing1 };

        // Copy-assignment operator
        PairingPQ<int> pairing3 {};
        pairing3 = pairing2;

        // A reference to a PairingPQ<T> is a reference to an Eecs281PQ<T>.
        // Yay for polymorphism! We can therefore write:
        Eecs281PQ<int> &pq1 = pairing1;
        Eecs281PQ<int> &pq2 = pairing2;
        Eecs281PQ<int> &pq3 = pairing3;  // NOLINT: Students may add testing that modifies pq3

        pq1.push(3);
        pq2.pop();
        assert(pq1.size() == 3);
        assert(not pq1.empty());
        assert(pq1.top() == 3);
        pq2.push(pq3.top());
        assert(pq2.top() == pq3.top());

        std::cout << "Basic tests done." << std::endl;

        // Remaining contents after the operations above
        assert(pq1.size() == 3);
        assert(pq2.size() == 2);
        assert(pq3.size() == 2);
        assert(pq3.top() == 1);

        // Copies must be independent
        pairing3.push(100);  // NOLINT
        assert(pairing3.top() == 100);
        assert(pairing2.top() != 100);

        // Self-assignment
        PairingPQ<int> &alias = pairing3;
        pairing3 = alias;
        assert(pairing3.top() == 100);
        assert(pairing3.size() == 3);

        // addNode / updateElt
        PairingPQ<int> heap {};
        std::vector<PairingPQ<int>::Node *> nodes;
        for (int v : { 10, 20, 30, 40, 50 }) {  // NOLINT
            nodes.push_back(heap.addNode(v));
        } // for
        assert(**nodes[2] == 30);
        assert(heap.top() == 50);

        heap.updateElt(nodes[0], 60);  // root child -> new top  NOLINT
        assert(heap.top() == 60);
        assert(nodes[0]->getElt() == 60);
        heap.updateElt(nodes[0], 70);  // already the root  NOLINT
        assert(heap.top() == 70);
        heap.pop();
        assert(heap.top() == 50);
        heap.updateElt(nodes[2], 55);  // deeper node after a pop  NOLINT
        assert(heap.top() == 55);
        for (int expected : { 55, 50, 40, 20 }) {
            assert(heap.top() == expected);
            heap.pop();
        } // for
        assert(heap.empty());

        // Many updateElt calls interleaved with pops
        PairingPQ<int> big {};
        std::vector<PairingPQ<int>::Node *> bigNodes;
        for (int i = 0; i < 1000; ++i) {  // NOLINT
            bigNodes.push_back(big.addNode(i));
        } // for
        for (int i = 0; i < 500; ++i) {  // NOLINT
            big.pop();  // removes 999 down to 500
        } // for
        for (int i = 0; i < 500; ++i) {  // NOLINT
            big.updateElt(bigNodes[static_cast<size_t>(i)], 2000 + i);  // NOLINT
        } // for
        for (int expected = 2499; expected >= 2000; --expected) {  // NOLINT
            assert(big.top() == expected);
            big.pop();
        } // for
        assert(big.empty());

        // Move construction / assignment
        PairingPQ<int> moved { std::move(pairing1) };
        assert(moved.size() == 3);
        assert(moved.top() == 3);
        PairingPQ<int> moveAssigned {};
        moveAssigned = std::move(moved);
        assert(moveAssigned.top() == 3);

        // That { above creates a scope, and our pairing heaps will fall out
        // of scope at the matching } below.
        std::cout << "Calling destructors" << std::endl;
    } // block for testing destructors

    std::cout << "testPairing succeeded!" << std::endl;
} // testPairing()


// Run all tests for a particular PQ type.
template <template <typename...> typename PQ>
void testPriorityQueue() {
    testPrimitiveOperations<PQ>();
    testHiddenData<PQ>();
    testUpdatePriorities<PQ>();
    testRandomAgainstUnordered<PQ, std::less<int>>();
    testRandomAgainstUnordered<PQ, std::greater<int>>();
} // testPriorityQueue()

// PairingPQ has some extra behavior we need to test in updateElement.
// This template specialization handles that without changing the nice
// uniform interface of testPriorityQueue.
template <>
void testPriorityQueue<PairingPQ>() {
    testPrimitiveOperations<PairingPQ>();
    testHiddenData<PairingPQ>();
    testUpdatePriorities<PairingPQ>();
    testRandomAgainstUnordered<PairingPQ, std::less<int>>();
    testRandomAgainstUnordered<PairingPQ, std::greater<int>>();
    testPairing();
} // testPriorityQueue<PairingPQ>()


int main() {
    const std::vector<PQType> types {
        PQType::Unordered,
        PQType::Sorted,
        PQType::Binary,
        PQType::Pairing,
    };

    std::cout << "PQ tester" << std::endl << std::endl;
    int idx { 0 };
    for (const auto &type : types) {
        std::cout << "  " << idx++ << ") " << type << std::endl;
    } // for
    std::cout << std::endl;

    std::cout << "Select one: ";
    uint32_t choice {};
    std::cin >> choice;
    const PQType pqType = types.at(choice);

    std::cout << "Testing the " << pqType << " PQ...";

    switch (pqType) {
    case PQType::Unordered:
        testPriorityQueue<UnorderedPQ>();
        break;
    case PQType::Sorted:
        testPriorityQueue<SortedPQ>();
        break;
    case PQType::Binary:
        testPriorityQueue<BinaryPQ>();
        break;
    case PQType::Pairing:
        testPriorityQueue<PairingPQ>();
        break;
    default:
        std::cout << "Unrecognized PQ type " << pqType << " in main.\n"
                  << "You must add tests for all PQ types." << std::endl;
        return 1;
    } // switch

    std::cout << "All tests succeeded!" << std::endl;

    return 0;
} // main()
