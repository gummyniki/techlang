#!/bin/zsh

# colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # no color

COMPILER="./build/techlang"
EXAMPLES_DIR="./examples"
PASSED=0
FAILED=0
ERRORS=0

echo "================================"
echo "   Techlang Test Suite"
echo "================================"
echo ""

# check compiler exists
if [ ! -f "$COMPILER" ]; then
    echo -e "${RED}Error: compiler not found at $COMPILER${NC}"
    echo "Run: mkdir build && cd build && cmake .. && make"
    exit 1
fi

run_test() {
    local tec_file=$1
    local expected_file=$2
    local test_name=$3

    # compile
    compile_output=$("$COMPILER" "$tec_file" 2>&1)
    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ $test_name${NC}"
        echo "   Compile error: $compile_output"
        ((ERRORS++))
        return
    fi

    # get the binary name (strip .tec extension)
    binary="${tec_file%.tec}"

    # run and capture output
    actual_output=$("$binary" 2>&1)
    expected_output=$(cat "$expected_file")

    # compare
    if [ "$actual_output" = "$expected_output" ]; then
        echo -e "${GREEN}✅ $test_name${NC}"
        ((PASSED++))
    else
        echo -e "${RED}❌ $test_name${NC}"
        echo "   Expected:"
        echo "$expected_output" | sed 's/^/     /'
        echo "   Got:"
        echo "$actual_output" | sed 's/^/     /'
        ((FAILED++))
    fi

    # clean up binary
    rm -f "$binary"
}

# run all tests
echo "Running tests..."
echo ""

run_test "$EXAMPLES_DIR/hello.tec"          		    "$EXAMPLES_DIR/hello.expected"         	       "Hello World"
run_test "$EXAMPLES_DIR/fibonacci.tec"      		    "$EXAMPLES_DIR/fibonacci.expected"      	       "Fibonacci"
run_test "$EXAMPLES_DIR/structs.tec"        		    "$EXAMPLES_DIR/structs.expected"        	       "Structs"
run_test "$EXAMPLES_DIR/enums.tec"          		    "$EXAMPLES_DIR/enums.expected"          	       "Enums"
run_test "$EXAMPLES_DIR/pointers.tec"       	            "$EXAMPLES_DIR/pointers.expected"       	       "Pointers"
run_test "$EXAMPLES_DIR/arrays.tec"         		    "$EXAMPLES_DIR/arrays.expected"         	       "Arrays"
run_test "$EXAMPLES_DIR/imports/main.tec"   		    "$EXAMPLES_DIR/imports/main.expected"   	       "Imports"
run_test "$EXAMPLES_DIR/casting.tec" 	    		    "$EXAMPLES_DIR/casting.expected"        	       "Casting"
run_test "$EXAMPLES_DIR/file_editing/files.tec" 	    "$EXAMPLES_DIR/file_editing/files.expected"        "Files"
run_test "$EXAMPLES_DIR/strings.tec" 	    		    "$EXAMPLES_DIR/strings.expected"        	       "Strings"
run_test "$EXAMPLES_DIR/gpu/sumReduce.tec" 	            "$EXAMPLES_DIR/gpu/sumReduce.expected"        	       "SumReduce"

# summary
echo ""
echo "================================"
echo -e "  ${GREEN}Passed: $PASSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "  ${RED}Failed: $FAILED${NC}"
fi
if [ $ERRORS -gt 0 ]; then
    echo -e "  ${YELLOW}Errors: $ERRORS${NC}"
fi
echo "================================"

# exit with failure if any tests failed
if [ $FAILED -gt 0 ] || [ $ERRORS -gt 0 ]; then
    exit 1
fi

exit 0
