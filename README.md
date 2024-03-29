# SIGMOD Programming Challenge

- [Link to Description](http://sigmodcontest2024.eastus.cloudapp.azure.com/task.shtml?content=description)
- [Link to Datasets](http://sigmodcontest2024.eastus.cloudapp.azure.com/task.shtml?content=datasets)

## Task Description

Given a set of vectors with additional attributes, the task is to answer hybrid vector search queries over the data accurately in limited time. A hybrid vector query is to find the approximate k nearest neighbors of a given query vector under one given similarity measure, such as Euclidean distance, with some constraints on non-vector attributes. For each query, your output should be the ids of the k nearest neighbors determined by your algorithm. For this year's task, k is set to be 100 and the vectors have a dimension of 100.

A sample dataset and a sample query set will be provided. The dataset contains millions of high-dimensional vectors, each also having a discretized categorical attribute (denoted as C) and a normalized timestamp attribute (denoted as T). We will refer to the dataset as D. The query set contains millions of hybrid vector queries. We will refer to it as Q.
Note: It is prohibitive to use query vectors during the indexing phase. Any submission that uses query information to create the index will result in the team being banned. After the contest, we will also conduct manual checks on the submissions of finalists.

## Dataset Structure

Dataset D is in a binary format, beginning with a 4-byte integer num_vectors (uint32_t) indicating the number of vectors. This is followed by data for each vector, stored consecutively, with each vector occupying 102 (2 + vector_num_dimension) x sizeof(float32) bytes, summing up to num_vectors x 102 (2 + vector_num_dimension) x sizeof(float32) bytes in total. Specifically, for the 102 dimensions of each vector: the first dimension denotes the discretized categorical attribute C and the second dimension denotes the normalized timestamp attribute T. The rest 100 dimensions are the vector.

## Query Set Structure

Query set Q is in a binary format, beginning with a 4-byte integer num_queries (uint32_t) indicating the number of queries. This is followed by data for each query, stored consecutively, with each query occupying 104 (4 + vector_num_dimension) x sizeof(float32) bytes, summing up to num_queries x 104 (4 + vector_num_dimension) x sizeof(float32) bytes in total.

The 104-dimensional representation for a query is organized as follows:

    The first dimension denotes query_type (takes values from 0, 1, 2, 3).
    The second dimension denotes the specific query value v for the categorical attribute (if not queried, takes -1).
    The third dimension denotes the specific query value l for the timestamp attribute (if not queried, takes -1).
    The fourth dimension denotes the specific query value r for the timestamp attribute (if not queried, takes -1).
    The rest 100 dimensions are the query vector.

There are four types of queries, i.e., the query_type takes values from 0, 1, 2 and 3. The 4 types of queries correspond to:

    If query_type=0: Vector-only query, i.e., the conventional approximate nearest neighbor (ANN) search query.
    If query_type=1: Vector query with categorical attribute constraint, i.e., ANN search for data points satisfying C=v.
    If query_type=2: Vector query with timestamp attribute constraint, i.e., ANN search for data points satisfying l≤T≤r.
    If query_type=3: Vector query with both categorical and timestamp attribute constraints, i.e. ANN search for data points satisfying C=v and l≤T≤r.

The predicate for the categorical attribute is an equality predicate, i.e., C=v. And the predicate for the timestamp attribute is a range predicate, i.e., l≤T≤r.

## I/O Instruction

We provide both the reading function(ReadBin) to load the dataset and the query set and the writing function(SaveBin) to generate the output file in the io.h file.

Your goal is to design an efficient algorithm for answering hybrid vector search queries. For each query, your output should be the ids of the k nearest neighbors determined by your algorithm. These neighbor lists are stored one by one and stored in a binary file.

During evaluation, we will replace the sample dataset D and sample query set Q with a hidden test set. The hidden test set is randomly drawn from the same distributions where D and Q were sampled from. We will evaluate your algorithms using the hidden test set. More details are available in the Evaluation section.

More details about the datasets can be found in the dedicated Datasets section.

output.bin format: The evaluation process expects "output.bin" to be a binary file containing |Q| x 100 x id (uint32_t). |Q| is the number of queries in query set Q, 100 is the number of nearest neighbors and id is the index of 100-nearest neighbors in the given dataset D.

Please format "output.bin" accordingly. You can check out our provided baseline solution on how to produce a valid "output.bin".

## Benchmarks

Current solution takes approximately:

- ~0.080 sec for dummy-*.bin
- ~41 sec for 1000 items of contest-*-release-1m.bin
- ~7 min for contest-*-release-1m.bin

## Approach & Ideas

For now i sort the database by C and then index it's values in a $C \rightarrow [start, end]$ mapping. Then if such criterion is used, i restrict the search on those intervals.

I would like to do something similar also for T, but i don't know how to join them.

For now also T is "indexed" by using the same sorting used for C and seeking for high/low range. This saves up to 65% of search time for the case of having both C(v) and T(l, r) in query. For reference, *contest* datasets now spends 2s vs old 28s on such type of queries with this shortcut.

I've optimized checks against *query_type* and now it takes slightly less than before. It's also more readable i guess.

I was thinking to indexing the vector space by dividing it in logical sections, peeking the most accurate for the query and then expanding the search to nearest sections.
