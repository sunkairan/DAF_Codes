all: test_pred_delay test_bats

test_pred_delay: test_pred_delay.cpp libbatscore.a
	g++ -g -c test_pred_delay.cpp -I../src -I../src/util -mtune=native -DPRINT_DEBUG -DSSE_DEBUG -DLOG_DEBUG
	g++ -g -o test_pred_delay test_pred_delay.o libbatscore.a

test_bats: test_bats.cpp libbatscore.a
	g++ -g -c test_bats.cpp -I../src -I../src/util -mtune=native -DPRINT_DEBUG -DSSE_DEBUG -DLOG_DEBUG
	g++ -g -o test_bats test_bats.o libbatscore.a

