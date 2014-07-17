<!--    Copyright (C) 2009-2014, Intel Corporation
        All rights reserved.
        
        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions
        are met:
        
          * Redistributions of source code must retain the above copyright
            notice, this list of conditions and the following disclaimer.
          * Redistributions in binary form must reproduce the above copyright
            notice, this list of conditions and the following disclaimer in
            the documentation and/or other materials provided with the
            distribution.
          * Neither the name of Intel Corporation nor the names of its
            contributors may be used to endorse or promote products derived
            from this software without specific prior written permission.
        
        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
        "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
        LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
        A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
        HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
        INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
        BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
        OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
        AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
        LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
        WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
        POSSIBILITY OF SUCH DAMAGE.
-->

@page pagereducers Intel Cilk Plus Reducers

<!--    Sections have been coded using Doxygen section / subsection /
        subsubsection tags rather than MarkDown ## / ### / #### markers to work
        around an apparent Doxygen bug in header processing. Change them back
        if the bug is ever fixed.
-->

[TOC]

@ingroup Reducers

@section reducers_intro Introduction

_Reducers_ address the problem of computing a value by incrementally updating a
variable in parallel code. Conceptually, a reducer is a variable that can be
safely used by multiple strands running in parallel. The runtime ensures that
each worker has access to a private instance of the variable, eliminating the
possibility of races without requiring locks. When parallel strands merge,
their variable instances are also merged ("reduced").

The Cilk library includes a general reducer framework and a collection of
predefined reducer classes to solve common specific problems. Many users will
find a predefined reducer class that meets their needs; but more advanced users
can use the framework to create new reducers to solve their problems.

@section reducers_examples Some Motivating Examples

@subsection reducers_example_add Avoiding Races

You might call this the "Hello, World" of reducer programs:

    #include <iostream>
    int main()
    {
        unsigned long sum = 0;
        for (int i = 0; i != 1000; i++) {
            sum += i*i;
        }
        std::cout << sum << "\n";
    }

This looks like it should be pretty easy to parallelize. Just throw in a
`cilk_for`, right?

    #include <iostream>
    #include <cilk/cilk.h>

    int main()
    {
        unsigned long sum = 0;
        cilk_for (int i = 0; i != 1000; i++) {
            sum += i*i;
        }
        std::cout << sum << "\n";
    }

Of course not! You would end up with multiple occurrences of `sum += i*i`
executing simultaneously. Presto: instant data race.

At this point, in traditional parallel programming, you would start thinking
about adding locks for the accumulator updates. Intel Cilk Plus has a more elegant
solution, though. Replace the accumulator variable with an Intel Cilk Plus reducer,
and Intel Cilk Plus will take care of the rest:

    #include <iostream>
    #include <cilk/cilk.h>
    #include <cilk/reducer_opadd.h>

    int main()
    {
        cilk::reducer< cilk::op_add<unsigned long> > sum;
        cilk_for (int i = 0; i != 1000; i++) {
            *sum += i*i;
        }
        std::cout << sum.get_value() << "\n";
    }

You just

*   use a reducer (`%cilk::reducer< cilk::op_add<unsigned long> >`) instead of
    a simple variable for the accumulator variable
*   treat the reducer like a pointer to the actual variable when you update it
    (`*sum += a[i]`)
*   get the final value out of the reducer when the computation is done
    (`sum.get_value()`)

And it all just works.

@subsection reducers_example_list Maintaining Order

That may not have seemed very impressive. Adding locks isn't all _that_ big
a bother. But coordinating a computation is often more of a problem than
avoiding the data races. Consider this example:

    #include <string>
    #include <iostream>
    #include <cilk/cilk.h>

    int main()
    {
        std::string alphabet;
        cilk_for(char letter = 'A'; letter <= 'Z'; ++letter) {
            alphabet += letter;
        }
        std::cout << alphabet << "\n";
    }

It has the same race problem as the previous example - simultaneous appends to
the string variable - but it also has a much worse problem. If you just add
locking code to keep the string appends from stepping on each other, you might
get output like:

    KPFGLABCUHQRSVWXDMTYZMEIJO

Locks makes sure that each update happens correctly, independent of all the
other updates, but they don't do a thing to make sure that they happen _in the
right order_. In fact, it might seem as though you would have to serialize the
tasks to combine the substrings in the right order, which would defeat the
point of parallelizing the program.

Reducers have a remarkable property, though: they are guaranteed to get the
same result in a parallel computation as in a serial computation. That is, even
in a parallel computation, they will combine all the input values in the same
order as in the serial computation!

You can make the same changes that you made to the first example:

    #include <string>
    #include <iostream>
    #include <cilk/cilk.h>
    #include <cilk/reducer_string.h>

    int main()
    {
        cilk::reducer<cilk::op_string> alphabet;
        cilk_for(char letter = 'A'; letter <= 'Z'; ++letter) {
            *alphabet += letter;
        }
        std::cout << alphabet.get_value() << "\n";
    }

When you run this program, it will always print

    ABCDEFGHIJKLMNOPQRSTUVWXYZ

@subsection reducers_examples_about A Note About These Examples

These examples are intended to illustrate the basic issues involved with
reducers. If you actually run them, you will probably find that your parallel
program runs slower than the one you started with. Intel Cilk Plus parallelism and
reducers are remarkably efficient, but they still have enough overhead to wipe
out the advantages of parallelizing ridiculously small tasks. (Alternatively,
you may find that the programs are so small that nothing actually executes in
parallel.)

In practice, algorithms that show a significant benefit from using reducers
will tend to have a loop or recursion where each step has a substantial amount
of work that can be done in parallel, and accumulates the result of that work
into the final result. Using a reducer for the accumulation solves the
synchronization and sequencing problems that would otherwise interfere with
parallelizing the entire algorithm.


@section reducers_how_they_work How Reducers Work

To understand how reducers work, it will be helpful to start with an
understanding of [the Cilk execution model](http://software.intel.com/sites/products/documentation/doclib/stdxe/2013/composerxe/compiler/cpp-win/index.htm#GUID-54163CE9-E866-4C6D-B0D4-0613DD2EA984.htm).

@subsection reducers_htw_reduction_algorithms Reduction Algorithms

Reducers are designed to support the parallel execution of a "reduction"
algorithm. Reduction algorithms fit the following pattern:

1.	There is an _accumulator variable_ _x_ with an _initial value_
    _a<sub>0</sub>_.
2.	There is a _reduction operation_ ⊕.
3.	⊕ is an _associative_ operation, i.e., _x ⊕ (y ⊕ z) = (x ⊕ y) ⊕ z_.
4.	⊕ has an _identity_ value _I_, i.e., _x ⊕ I = I ⊕ x = x_.
5.	The code repeatedly updates the variable, with each update having the form
    _x = x ⊕ a<sub>i</sub>_. After _N_ updates, _x_ contains
    _a<sub>0</sub> ⊕ a<sub>1</sub> ⊕ a<sub>2</sub> ⊕ … ⊕ a<sub>N</sub>_.

@subsection reducers_htw_monoids Monoids

A set of values with an associative operation and an identity value is referred
to in mathematics as a "monoid". Common operations that fit this pattern
include addition and multiplication, bitwise AND and OR, set union,
list concatenation, and string concatenation. Properties 2, 3, and 4 of
reduction algorithms mean that a reduction algorithm always has an underlying
monoid.

@subsection reducers_htw_views Views

A reducer object manages multiple copies of an accumulator variable, called
"views," as strands of execution are spawned and synced. The basic idea is that
each strand gets its own view, so strands executing concurrently can update
their views independently, without data races.

1.	Initially, a reducer contains a single view, called the "leftmost" view.
2.	A new view is created for the continuation of each spawn. (The child of the
    spawn inherits the view from the spawning strand.) The new view is
    initialized to the reduction operation's identity value.
3.	Within a strand, all accesses to the reducer's content refer to the view
    that was created for that strand.
4.	When two adjacent strands are synced, the reduction operation is applied to
    their views, leaving the result in the left strand's view, which is
    inherited by the synced strand. The right strand's view is then destroyed.
5.	When all strands have been synced, the final result of the computation
    remains in the leftmost view.

As a result of this process, each strand computes a subsequence of the total
sequence of operations
(_a<sub>i</sub> ⊕ a<sub>i+1</sub> ⊕ … ⊕ a<sub>i+m</sub>_)
in  its view, and when the computation is finished, the leftmost view contains
the expected result
(a<sub>0</sub> ⊕ a<sub>1</sub> ⊕ a<sub>2</sub> ⊕ … ⊕ a<sub>N</sub>).
The order of the values is the same as in the serial computation, and since
the operation is associative, it doesn't matter what subsequence of the
computation is performed by each strand.

@subsection reducers_htw_example An Example

Let's work through an example.

    #include <iostream>
    #include <cilk/cilk.h>
    #include <cilk/reducer_string.h>

    typedef cilk::reducer< cilk::op_or<unsigned long> > Reducer;

    int main()
    {
        Reducer a("((");            // 1
        cilk_spawn abcd(a);         // 2
                   efgh(a);         // 3
        cilk_sync;                  // 14
        a->append("))");            // 15
        std::cout << a.get_value()
                  << "\n";
    }

    void abcd(Reducer& x)
    {
        cilk_spawn ab(x);           // 3
                   cd(x);           // 5
        cilk_sync;                  // 7
    }

    void ab(Reducer& x)
    {
        x->append("a");             // 4
        x->append("b");
    }

    void cd(Reducer& x)
    {
        x->append("c");             // 6
        x->append("d");
    }

    void efgh(Reducer& x)
    {
        cilk_spawn ef(x);           // 9
                   gh(x);           // 11
        cilk_sync;                  // 13
    }

    void ef(Reducer& x)
    {
        x->append("e");             // 10
        x->append("f");
    }

    void gh(Reducer& x)
    {
        x->append("g");             // 12
        x->append("h");
    }

The reduction operation for a string reducer is string concatenation, which is
an associative operation. (The view operation `view.append(x)` is equivalent to
`view = view + string(x)`.) The identity value is the empty string.

1.  The constructor creates a reducer with a leftmost view initialized
    to `"(("`.
2.  The spawned strand for `abcd()` inherits the leftmost view.
3.  The spawned strand for `ab()` inherits the leftmost view.
4.  `ab()` updates the value of the leftmost view first to `"((a"`,
    then to `"((ab"`.
5.  `cd()` runs in the continuation strand, which gets a new view,
    initialized to the string identity value, `""`. Call this view 1.
6.  `cd()` updates the value of view 1 first to `"c"`, then to `"cd"`.
7.  The leftmost view and view 1 are reduced: the value of view 1 is
    appended to the leftmost view, leaving `"((abcd"` in the leftmost view.
    View 1 is destroyed, and the strand which returns from `abcd()` inherits
    the leftmost view.
8.  `efgh()` runs in the continuation strand, which gets a new view,
    initialized to the string identity value, `""`. Call this view 2.
9.  The spawned strand for `ef()` inherits view 2.
10. `ef()` updates the value of view 2 first to `"e"`, then to `"ef"`.
11. `gh()` runs in the continuation strand, which gets a new view,
    initialized to the string identity value, `""`. Call this view 3.
12. `gh()` updates the value of view 3 first to `"g"`, then to `"gh"`.
13.  View 2 and view 3 are reduced: the value of view 3 is appended to view 2,
    leaving `"efgh"` in view 2. View 3 is destroyed, and the strand which
    returns from `efgh()` inherits view 2.
14. The leftmost view and view 2 are reduced: the value of view 2 is
    appended to the leftmost view, leaving `"((abcdefgh"` in the leftmost view.
    View 2 is destroyed, and the strand which continues from the `sync`
    inherits the leftmost view.
15. The value of the leftmost view is updated to `"((abcdefgh))"`, and that is
    what is printed.

@subsection reducers_htw_technical Technical Details

The description of view management [above](#reducers_htw_views) is simplified
considerably. This section provides the actual view creation and merging rules,
as well as an explanation of why having an underlying monoid is necessary to guarantee correct reducer behavior.

You can safely skip ahead to [Using Reducers](#reducers_using) if you aren't
interested in these details.

@subsubsection reducers_htw_terminology Terminology

@paragraph reducers_htw_graph The Parallelism Graph

The parallelism in an execution of a Cilk program can be represented by a
_parallelism graph_. This is a directed acyclic graph (DAG) whose vertices
represent the parallelism events in the program execution and whose edges
represent the serial execution sequences between parallelism events, which
we refer as the _strands_ of the program execution. The kinds of vertices are:

*   __Begin__: The graph has a single begin vertex, which represents the start
    of the program execution. The begin vertex has no incoming edges and one
    outgoing edge, which represents the first strand in the program
    execution.
*   __End__: The graph has a single end vertex, which represents the end of the
    program execution. The end vertex has one incoming edge, which represents
    the last strand in the program execution, and no outgoing edges.
*   __Spawn__: A spawn vertex represents an execution of a `cilk_spawn`. It has
    one incoming edge, representing the strand which ends with the execution of
    the `cilk_spawn`, and two outgoing edges, the _child_ edge, representing
    the beginning of the spawned function, and the _continuation_ edge,
    representing the strand immediately following the `cilk_spawn`.
*   __Sync__: A sync vertex represents the execution of a `cilk_sync`. It has
    one or more input edges, representing the strands which must complete
    before execution can continue (including the strand which ends with the
    execution of the `cilk_sync`), and one outgoing edge, representing the
    strand which begins executing after all of the incoming strands have
    completed.

(This representation works equally well for programs with `cilk_for` loops,
which are implemented by spawning groups of loop iterations and then syncing
after the loop.)

Every spawn vertex is associated with a unique sync vertex, but a sync vertex
can be associated with multiple spawn vertices.

Note that the parallelism graph is a representation of a particular _execution_
of a program, not a representation of the source program structure like
a traditional control flow graph. Also note that the graph represents the
_available_ parallelism in the execution - it is independent of what steals
actually occur during the execution.

@paragraph reducers_htw_order Strand Ordering

We say that two strands are _serially ordered_ if there is a path in the
parallelism graph that contains the edges representing both strands. This means
that regardless of what steals occur during the execution, one of the strands
will be completely executed before the other.

Conversely, if there is _no_ path in the parallelism graph that contains the
edges representing both strands, then the strands are _parallel_, and could
execute concurrently, depending on what steals occur. This will be the case if
one of the edges occurs on a path from some spawn's child edge to its
corresponding sync, and the other edge occurs on a path from the same spawn's
continuation edge to the same sync.

If two strands are parallel, we say that the one on the path from the child
edge is _to the left of_ the one on the path from the continuation edge. (The
left strand would be executed before the right strand if the program were
executed serially.) If strand A is to the left of strand B, and strand B is to
the left of strand C, then strand A is to the left of strand C.

Two strands are _adjacent_ if they are parallel and there is no other strand
which is to the left of one of them and to the right of the other.

@subsubsection reducers_htw_view_details View Management Details

@paragraph reducers_htw_views_lazy Lazy View Creation

The reason for creating private view instances for strands is to eliminate data
races. A strand does not really need its own instance of a view unless (1) it
actually accesses the view, and (2) it executes in parallel with some other
strand. Creating, managing, and merging views is not terribly expensive, but it
isn't free, so the scheduler avoids creating views unnecessarily. The actual
view creation rule is:

1.  When a reducer is created, a leftmost view is created for it, which becomes
    the view for the strand it was created on.
2.  When a spawn occurs, both the spawned strand and the continuation inherit
    the spawning strand's view (if it has one).
3.  When a continuation is stolen, its view reference is erased; it then does
    not have any view.
4.  When an attempt is made to access the view of a strand that does not have a
    view, a new view is created for that strand, and initialized to the
    identity value.

@paragraph reducers_htw_views_merging Merging Views

When a strand terminates, its view is saved until an adjacent strand has also
terminated. When two adjacent strands have both terminated, their views are
merged:

1.  If neither strand had a view, then there is still no view.
2.  If only one strand had a view, then that is the merged view.
3.  If both strands had views, then their value are combined with the reduction
    operation. The resulting value is left in the left view, and the right
    view is destroyed.

When all the strands entering a `cilk_sync` have terminated and their views
have been merged, the merged view becomes the view of the strand coming out of
the `cilk_sync`. This will be the same as the view of the strand that entered
the `cilk_spawn`.

It is guaranteed that if a continuation is _not_ stolen, then no steals will
occur in the spawned function, either, so when the spawned function returns,
the continuation executes using the same view as the spawned function. In this
case, there is no waiting and no merge at the `cilk_sync`.

@subsubsection reducers_htw_why_monoid Why Reducers Need a Monoid

We said [above](#reducers_htw_monoids) that every reducer is built around
a monoid, which is a set of values with an associative operation and an
identity value. Let's look at some sample code to see why the identity value
and the associativity property are both necessary to guarantee that parallel
execution using the reducer will give the same result as serial execution with
a simple accumulator variable.

    void t1(cilk::reducer<cilk::op_string> &r) {
        *r += 'A';
        *r += 'B';
        ...
        *r += 'G';
    }
    void t2(cilk::reducer<cilk::op_string> &r) {
        *r += 'H';
        *r += 'I';
        ...
        *r += 'M';
    }
    void t3(cilk::reducer<cilk::op_string> &r) {
        *r += 'N';
        *r += 'O';
        ...
        *r += 'T';
    }
    void t4(cilk::reducer<cilk::op_string> &r) {
        *r += 'U';
        *r += 'V';
        ...
        *r += 'Z';
    }

    int main() {
        cilk::reducer<cilk::op_string> r;
        cilk_spawn t1(r);
        cilk_spawn t2(r);
        cilk_spawn t3(r);
        t4(r);
        cilk_sync;
        cout << r.get_value() << "\n";
    }

Suppose that all of the continuations are stolen, so that `t1`, `t2`, `t3`, and
`t4` execute in parallel and each get their own views. The left-to-right
ordering will be `t1`, `t2`, `t3`, `t4`.

@paragraph reducers_htw_why_identity Identity

In the serial execution of the program, the reducer would contain `"ABCDEFG"`
after the assignments in `t1()` and `"ABCDEFGHIJKLM"` after the assignments in
`t2()`. In the parallel execution, `t1()` and `t2()` execute on separate
strands which each get their own views, which we expect to contain `"ABCDEFG"`
and `"HIJKLM"` when they reach the `cilk_sync`. `t1` has the leftmost view,
which is initialized to the empty string when the reducer is created. `t2`'s
view is created when the continuation of the `cilk_spawn t1()` is stolen. For
`t2`'s view to contain `"HIJKLM"` at the end of `t2`, it must be initialized to
the string concatenation identity value, which is the empty string, when it is
created.

More generally, if a non-leftmost strand executes the assignments

    *r = *r ⊕ a1
    *r = *r ⊕ a2
    ...
    *r = *r ⊕ an

then we expect the final value of its view to be `a1 ⊕ a2 ⊕ ... ⊕ an`. If its
initial value is `x`, then its final value will actually be
`x ⊕ a1 ⊕ a2 ⊕ ... ⊕ an`, which is equal to `a1 ⊕ a2 ⊕ ... ⊕ an` only if
`x` is the identity value for the `⊕` operation.

In other words, if the reducer's operator did not have an identity value, then
there would be no way to give a non-leftmost view an initial value such that
it ended up with the correct final value.

@paragraph reducers_htw_why_associativity Associativity

There are two sources of non-determinism in parallel execution.

First, how the serial sequence of reducer operations is split up into parallel
subsequences depends on which spawn continuations are stolen (and,
consequently, [what views are created](#reducers_htw_views_lazy)). In our
example:

*   If no steals occur, then there will be only a single view containing the
    sequence (A–Z).
*   If one of the three possible steals occurs, then there will be two views
    containing subsequences (A–G, H–Z), (A–M, N–Z), or (A–T, U–Z).
*   If two steals occurs, then there will be three views containing
    subsequences (A–G, H–M, N–Z), (A–G, H–T, U–Z), or (A–M, N–T, U–Z).
*   If all three steals occur, then there will be four views containing
    subsequences (A–G, H–M, N–T, U–Z).

Second, the order in which the results of parallel computations (i.e., the
values of the views) are [combined](#reducers_htw_views_merging) depends on
the order in which the parallel strands reach the sync point. Considering our
example with three steals and four views again:

*   If the strands terminate in order (`t1`, `t2`, `t3`, `t4`), then the
    subsequence results will be combined in the order (((A–G, H–M), N–T), U–Z).
*   If the strands terminate in order (`t4`, `t3`, `t2`, `t1`), then the
    subsequence results will be combined in the order (A–G, (H–M, (N–T, U–Z))).
*   If the strands terminate in order (`t3`, `t1`, `t4`, `t2`), then the
    subsequence results will be combined in the order ((A–G, H–M), (N–T, U–Z)).

Note that both kinds of non-determinism re-associate the reducer operations while
leaving their left-to-right order unchanged. Thus, the final result in the
reducer after a parallel computation is guaranteed to be the same as after a
serial computation if and only if the reducer operations are associative.

@section reducers_how_it_works How It Works

Reductions are accomplished by the collaboration of three components: a monoid,
a view, and a reducer.

-   The _monoid_ is an object of a class that models the `Monoid` concept. It
    represents the mathematical monoid underlying the reduction, by providing
    -   definitions of the value and view types, and
    -   operations to manage memory for views, to initialize a view with the
        identity value (_x<sub>j</sub> = I_), and to combine the values of two
        views (_x<sub>j</sub> = x<sub>j</sub> ⊕ x<sub>j+1)</sub>_).
-   The _view_ is a wrapper for the accumulator variable. Since all access to
    the accumulator is through the view, it can ensure that all modifications
    to the accumulator variable are consistent with the operation of the
    underlying monoid (_x<sub>j</sub> = x<sub>j</sub> ⊕ a<sub>i)</sub>_). For
    example, an addition view `v` would allow `v = v + x` and `v++`, but not
    `v = -v` or  `v = v * 3`.
-   The _reducer_ is an object of the `cilk::reducer` template class,
    instantiated with the monoid class. The reducer object contains the monoid
    object and the view object as data members.

@subsection reducers_monoid_concept The `Monoid` Concept
    \warning To be documented...

@section reducers_using Using Reducers
    \warning To be documented...

@section reducers_creating Creating New Reducers
    \warning To be documented...

@section reducers_in_c Reducers in C

Reducers can also be created and used (rather less elegantly) in C.
See @subpage page_reducers_in_c.

@section reducers_library Library Reducers

**Arithmetic Reducers**
-   @subpage ReducersAdd
-   @subpage ReducersMul
-   @subpage ReducersMinMax
-   @subpage ReducersAnd
-   @subpage ReducersOr
-   @subpage ReducersXor

**Container Reducers**
-   @subpage ReducersList
<!--
-   @subpage ReducersMap
-   @subpage ReducersSet
-->
-   @subpage ReducersString
-   @subpage ReducersVector

**Other Reducers**
-   @subpage ReducersOstream

@subsection reducers_legacy Legacy Reducer Wrappers
    \warning To be documented...
