@all: builddir/main.exe

builddir/src/sigmod/query_set.o: src/sigmod/query_set.cc include/sigmod/query_set.hh include/sigmod/config.hh include/sigmod/stats.hh include/sigmod/query.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/query_set.o -c src/sigmod/query_set.cc

builddir/src/sigmod/random.o: src/sigmod/random.cc include/sigmod/config.hh include/sigmod/random.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/random.o -c src/sigmod/random.cc

builddir/src/sigmod/record.o: src/sigmod/record.cc include/sigmod/config.hh include/sigmod/record.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/record.o -c src/sigmod/record.cc

builddir/src/sigmod/stats.o: src/sigmod/stats.cc include/sigmod/config.hh include/sigmod/stats.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/stats.o -c src/sigmod/stats.cc

builddir/src/sigmod/scoreboard.o: src/sigmod/scoreboard.cc include/sigmod/scoreboard.hh include/sigmod/flags.hh include/sigmod/record.hh include/sigmod/config.hh include/sigmod/query.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/scoreboard.o -c src/sigmod/scoreboard.cc

builddir/src/sigmod/ball_tree.o: src/sigmod/ball_tree.cc include/sigmod/scoreboard.hh include/sigmod/ball_tree.hh include/sigmod/solution.hh include/sigmod/database.hh include/sigmod/query.hh include/sigmod/flags.hh include/sigmod/tweaks.hh include/sigmod/debug.hh include/sigmod/record.hh include/sigmod/config.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/ball_tree.o -c src/sigmod/ball_tree.cc

builddir/src/sigmod/solution.o: src/sigmod/solution.cc include/sigmod/flags.hh include/sigmod/scoreboard.hh include/sigmod/solution.hh include/sigmod/database.hh include/sigmod/stats.hh include/sigmod/query_set.hh include/sigmod/record.hh include/sigmod/config.hh include/sigmod/query.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/solution.o -c src/sigmod/solution.cc

builddir/src/sigmod/debug.o: src/sigmod/debug.cc include/sigmod/debug.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/debug.o -c src/sigmod/debug.cc

builddir/src/sigmod/exaustive.o: src/sigmod/exaustive.cc include/sigmod/scoreboard.hh include/sigmod/solution.hh include/sigmod/database.hh include/sigmod/query.hh include/sigmod/flags.hh include/sigmod/exaustive.hh include/sigmod/seek.hh include/sigmod/debug.hh include/sigmod/record.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/exaustive.o -c src/sigmod/exaustive.cc

builddir/src/sigmod/c_map.o: src/sigmod/c_map.cc include/sigmod/config.hh include/sigmod/c_map.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/c_map.o -c src/sigmod/c_map.cc

builddir/src/sigmod/database.o: src/sigmod/database.cc include/sigmod/c_map.hh include/sigmod/config.hh include/sigmod/record.hh include/sigmod/database.hh include/sigmod/stats.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/database.o -c src/sigmod/database.cc

builddir/src/sigmod/query.o: src/sigmod/query.cc include/sigmod/config.hh include/sigmod/query.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/query.o -c src/sigmod/query.cc

builddir/src/sigmod/workflow.o: src/sigmod/workflow.cc include/sigmod/flags.hh include/sigmod/scoreboard.hh include/sigmod/ball_tree.hh include/sigmod/solution.hh include/sigmod/seek.hh include/sigmod/exaustive.hh include/sigmod/database.hh include/sigmod/debug.hh include/sigmod/query_set.hh include/sigmod/workflow.hh include/sigmod/c_map.hh include/sigmod/record.hh include/sigmod/config.hh include/sigmod/query.hh include/sigmod/kd_tree.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/workflow.o -c src/sigmod/workflow.cc

builddir/src/sigmod/kd_tree.o: src/sigmod/kd_tree.cc include/sigmod/scoreboard.hh include/sigmod/flags.hh include/sigmod/solution.hh include/sigmod/database.hh include/sigmod/debug.hh include/sigmod/random.hh include/sigmod/record.hh include/sigmod/config.hh include/sigmod/query.hh include/sigmod/kd_tree.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/sigmod/kd_tree.o -c src/sigmod/kd_tree.cc

builddir/src/main.o: src/main.cc include/sigmod/solution.hh include/sigmod/database.hh include/sigmod/stats.hh include/sigmod/query_set.hh include/sigmod/workflow.hh include/sigmod/record.hh include/sigmod/config.hh include/sigmod.hh include/sigmod/query.hh
	g++ -Iinclude --std=c++17 -O3 -o builddir/src/main.o -c src/main.cc

builddir/main.exe: builddir/src/sigmod/query_set.o builddir/src/sigmod/random.o builddir/src/sigmod/record.o builddir/src/sigmod/stats.o builddir/src/sigmod/scoreboard.o builddir/src/sigmod/ball_tree.o builddir/src/sigmod/solution.o builddir/src/sigmod/debug.o builddir/src/sigmod/exaustive.o builddir/src/sigmod/c_map.o builddir/src/sigmod/database.o builddir/src/sigmod/query.o builddir/src/sigmod/workflow.o builddir/src/sigmod/kd_tree.o builddir/src/main.o
	g++ -Iinclude --std=c++17 -O3 -o builddir/main.exe builddir/src/sigmod/query_set.o builddir/src/sigmod/random.o builddir/src/sigmod/record.o builddir/src/sigmod/stats.o builddir/src/sigmod/scoreboard.o builddir/src/sigmod/ball_tree.o builddir/src/sigmod/solution.o builddir/src/sigmod/debug.o builddir/src/sigmod/exaustive.o builddir/src/sigmod/c_map.o builddir/src/sigmod/database.o builddir/src/sigmod/query.o builddir/src/sigmod/workflow.o builddir/src/sigmod/kd_tree.o builddir/src/main.o

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
	./builddir/main.exe contest-1m-data.bin contest-1m-queries.bin

contest-10m: builddir/main.exe
	./builddir/main.exe contest-10m-data.bin contest-10m-queries.bin

