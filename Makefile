@all: builddir/main.exe

builddir/src/main.o: src/main.cc include/sigmod/flags.hh include/sigmod/stats.hh include/sigmod/solution.hh include/sigmod.hh include/sigmod/config.hh include/sigmod/database.hh include/sigmod/query_set.hh include/sigmod/workflow.hh include/sigmod/query.hh include/sigmod/record.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/main.o -c src/main.cc

builddir/src/sigmod/ball_tree.o: src/sigmod/ball_tree.cc include/sigmod/config.hh include/sigmod/debug.hh include/sigmod/database.hh include/sigmod/tree_utils.hh include/sigmod/query.hh include/sigmod/scoreboard.hh include/sigmod/record.hh include/sigmod/tweaks.hh include/sigmod/flags.hh include/sigmod/thread_pool.hh include/sigmod/solution.hh include/sigmod/ball_tree.hh include/sigmod/c_map.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/ball_tree.o -c src/sigmod/ball_tree.cc

builddir/src/sigmod/config.o: src/sigmod/config.cc include/sigmod/config.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/config.o -c src/sigmod/config.cc

builddir/src/sigmod/c_map.o: src/sigmod/c_map.cc include/sigmod/c_map.hh include/sigmod/config.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/c_map.o -c src/sigmod/c_map.cc

builddir/src/sigmod/database.o: src/sigmod/database.cc include/sigmod/tweaks.hh include/sigmod/flags.hh include/sigmod/random_projection.hh include/sigmod/stats.hh include/sigmod/config.hh include/sigmod/pca.hh include/sigmod/debug.hh include/sigmod/database.hh include/sigmod/lin_alg.hh include/sigmod/memory.hh include/sigmod/c_map.hh include/sigmod/dimensional_reduction.hh include/sigmod/record.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/database.o -c src/sigmod/database.cc

builddir/src/sigmod/debug.o: src/sigmod/debug.cc include/sigmod/debug.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/debug.o -c src/sigmod/debug.cc

builddir/src/sigmod/exaustive.o: src/sigmod/exaustive.cc include/sigmod/seek.hh include/sigmod/debug.hh include/sigmod/database.hh include/sigmod/query.hh include/sigmod/record.hh include/sigmod/scoreboard.hh include/sigmod/flags.hh include/sigmod/solution.hh include/sigmod/exaustive.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/exaustive.o -c src/sigmod/exaustive.cc

builddir/src/sigmod/kd_tree.o: src/sigmod/kd_tree.cc include/sigmod/flags.hh include/sigmod/solution.hh include/sigmod/kd_tree.hh include/sigmod/config.hh include/sigmod/random.hh include/sigmod/debug.hh include/sigmod/tree_utils.hh include/sigmod/database.hh include/sigmod/record.hh include/sigmod/query_set.hh include/sigmod/query.hh include/sigmod/scoreboard.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/kd_tree.o -c src/sigmod/kd_tree.cc

builddir/src/sigmod/lin_alg.o: src/sigmod/lin_alg.cc include/sigmod/config.hh include/sigmod/lin_alg.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/lin_alg.o -c src/sigmod/lin_alg.cc

builddir/src/sigmod/lsh.o: src/sigmod/lsh.cc include/sigmod/flags.hh include/sigmod/config.hh include/sigmod/debug.hh include/sigmod/database.hh include/sigmod/record.hh include/sigmod/lsh.hh include/sigmod/memory.hh include/sigmod/query.hh include/sigmod/scoreboard.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/lsh.o -c src/sigmod/lsh.cc

builddir/src/sigmod/mvp_tree.o: src/sigmod/mvp_tree.cc include/sigmod/tweaks.hh include/sigmod/thread_pool.hh include/sigmod/solution.hh include/sigmod/config.hh include/sigmod/tree_utils.hh include/sigmod/mvp_tree.hh include/sigmod/memory.hh include/sigmod/c_map.hh include/sigmod/query.hh include/sigmod/scoreboard.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/mvp_tree.o -c src/sigmod/mvp_tree.cc

builddir/src/sigmod/pca.o: src/sigmod/pca.cc include/sigmod/pca.hh include/sigmod/random_projection.hh include/sigmod/lin_alg.hh include/sigmod/config.hh include/sigmod/dimensional_reduction.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/pca.o -c src/sigmod/pca.cc

builddir/src/sigmod/query.o: src/sigmod/query.cc include/sigmod/config.hh include/sigmod/query.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/query.o -c src/sigmod/query.cc

builddir/src/sigmod/query_set.o: src/sigmod/query_set.cc include/sigmod/tweaks.hh include/sigmod/random_projection.hh include/sigmod/stats.hh include/sigmod/config.hh include/sigmod/database.hh include/sigmod/lin_alg.hh include/sigmod/query_set.hh include/sigmod/query.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/query_set.o -c src/sigmod/query_set.cc

builddir/src/sigmod/random.o: src/sigmod/random.cc include/sigmod/random.hh include/sigmod/config.hh include/sigmod/query_set.hh include/sigmod/database.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/random.o -c src/sigmod/random.cc

builddir/src/sigmod/random_projection.o: src/sigmod/random_projection.cc include/sigmod/random_projection.hh include/sigmod/database.hh include/sigmod/lin_alg.hh include/sigmod/query_set.hh include/sigmod/config.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/random_projection.o -c src/sigmod/random_projection.cc

builddir/src/sigmod/record.o: src/sigmod/record.cc include/sigmod/flags.hh include/sigmod/config.hh include/sigmod/record.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/record.o -c src/sigmod/record.cc

builddir/src/sigmod/scoreboard.o: src/sigmod/scoreboard.cc include/sigmod/flags.hh include/sigmod/debug.hh include/sigmod/record.hh include/sigmod/config.hh include/sigmod/query.hh include/sigmod/scoreboard.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/scoreboard.o -c src/sigmod/scoreboard.cc

builddir/src/sigmod/solution.o: src/sigmod/solution.cc include/sigmod/tweaks.hh include/sigmod/flags.hh include/sigmod/solution.hh include/sigmod/stats.hh include/sigmod/config.hh include/sigmod/debug.hh include/sigmod/database.hh include/sigmod/record.hh include/sigmod/query_set.hh include/sigmod/query.hh include/sigmod/scoreboard.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/solution.o -c src/sigmod/solution.cc

builddir/src/sigmod/statistical_indeces.o: src/sigmod/statistical_indeces.cc include/sigmod/config.hh include/sigmod/database.hh include/sigmod/statistical_indeces.hh include/sigmod/lin_alg.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/statistical_indeces.o -c src/sigmod/statistical_indeces.cc

builddir/src/sigmod/stats.o: src/sigmod/stats.cc include/sigmod/config.hh include/sigmod/stats.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/stats.o -c src/sigmod/stats.cc

builddir/src/sigmod/thread_pool.o: src/sigmod/thread_pool.cc include/sigmod/tweaks.hh include/sigmod/c_map.hh include/sigmod/thread_pool.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/thread_pool.o -c src/sigmod/thread_pool.cc

builddir/src/sigmod/tree_utils.o: src/sigmod/tree_utils.cc include/sigmod/flags.hh include/sigmod/config.hh include/sigmod/debug.hh include/sigmod/database.hh include/sigmod/record.hh include/sigmod/tree_utils.hh include/sigmod/query.hh include/sigmod/scoreboard.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/tree_utils.o -c src/sigmod/tree_utils.cc

builddir/src/sigmod/vp_tree.o: src/sigmod/vp_tree.cc include/sigmod/vp_tree.hh include/sigmod/config.hh include/sigmod/debug.hh include/sigmod/database.hh include/sigmod/query.hh include/sigmod/scoreboard.hh include/sigmod/record.hh include/sigmod/tweaks.hh include/sigmod/flags.hh include/sigmod/solution.hh include/sigmod/random.hh include/sigmod/query_set.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/vp_tree.o -c src/sigmod/vp_tree.cc

builddir/src/sigmod/workflow.o: src/sigmod/workflow.cc include/sigmod/random_projection.hh include/sigmod/memory.hh include/sigmod/vp_tree.hh include/sigmod/kd_tree.hh include/sigmod/config.hh include/sigmod/seek.hh include/sigmod/exaustive.hh include/sigmod/debug.hh include/sigmod/database.hh include/sigmod/mvp_tree.hh include/sigmod/tree_utils.hh include/sigmod/lin_alg.hh include/sigmod/query.hh include/sigmod/scoreboard.hh include/sigmod/record.hh include/sigmod/flags.hh include/sigmod/solution.hh include/sigmod/ball_tree.hh include/sigmod/lsh.hh include/sigmod/query_set.hh include/sigmod/c_map.hh include/sigmod/workflow.hh
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/src/sigmod/workflow.o -c src/sigmod/workflow.cc

builddir/main.exe: builddir/src/main.o builddir/src/sigmod/ball_tree.o builddir/src/sigmod/config.o builddir/src/sigmod/c_map.o builddir/src/sigmod/database.o builddir/src/sigmod/debug.o builddir/src/sigmod/exaustive.o builddir/src/sigmod/kd_tree.o builddir/src/sigmod/lin_alg.o builddir/src/sigmod/lsh.o builddir/src/sigmod/mvp_tree.o builddir/src/sigmod/pca.o builddir/src/sigmod/query.o builddir/src/sigmod/query_set.o builddir/src/sigmod/random.o builddir/src/sigmod/random_projection.o builddir/src/sigmod/record.o builddir/src/sigmod/scoreboard.o builddir/src/sigmod/solution.o builddir/src/sigmod/statistical_indeces.o builddir/src/sigmod/stats.o builddir/src/sigmod/thread_pool.o builddir/src/sigmod/tree_utils.o builddir/src/sigmod/vp_tree.o builddir/src/sigmod/workflow.o
	g++ -Iinclude --std=c++17 -O3 -pthread -fopenmp -fstack-protector-all -o builddir/main.exe builddir/src/main.o builddir/src/sigmod/ball_tree.o builddir/src/sigmod/config.o builddir/src/sigmod/c_map.o builddir/src/sigmod/database.o builddir/src/sigmod/debug.o builddir/src/sigmod/exaustive.o builddir/src/sigmod/kd_tree.o builddir/src/sigmod/lin_alg.o builddir/src/sigmod/lsh.o builddir/src/sigmod/mvp_tree.o builddir/src/sigmod/pca.o builddir/src/sigmod/query.o builddir/src/sigmod/query_set.o builddir/src/sigmod/random.o builddir/src/sigmod/random_projection.o builddir/src/sigmod/record.o builddir/src/sigmod/scoreboard.o builddir/src/sigmod/solution.o builddir/src/sigmod/statistical_indeces.o builddir/src/sigmod/stats.o builddir/src/sigmod/thread_pool.o builddir/src/sigmod/tree_utils.o builddir/src/sigmod/vp_tree.o builddir/src/sigmod/workflow.o

clean:
	rm -rf builddir
	mkdir builddir
	mkdir builddir/src
	mkdir builddir/src/sigmod

run: builddir/main.exe
	./builddir/main.exe 

dummy: builddir/main.exe
	./builddir/main.exe dummy-data.bin dummy-queries.bin

contest-1m: builddir/main.exe
	./builddir/main.exe contest-data-release-1m.bin contest-queries-release-1m.bin

contest-10m: builddir/main.exe
	./builddir/main.exe contest-data-release-10m.bin contest-queries-release-10m.bin

