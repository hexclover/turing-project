OUT=xxx-xxx.tar.gz

all:
	cd ./turing-project && make clean
	-mv $(OUT) $(OUT).bak
	tar zcvf $(OUT) --exclude="*~" --exclude=".cache" --exclude "compile_commands.json" programs turing-project README.md xxx-xxx.pdf
	docker build -t fla-test .
	docker run fla-test

run-tests:
	cd ./turing-project && ./test-gcd.sh
