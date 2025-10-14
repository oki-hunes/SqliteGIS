#!/bin/bash
# ============================================================================
# SqliteGIS Test Runner
# ============================================================================
# 
# This script runs all test suites for the SqliteGIS extension.
# Usage: ./tests/run_tests.sh
#
# Requirements:
# - SQLite3 with loadable extension support
# - sqlitegis.dylib built in Workspace/build/
# ============================================================================

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
WORKSPACE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$WORKSPACE_DIR/build"
TESTS_DIR="$WORKSPACE_DIR/tests"
EXTENSION_LIB="$BUILD_DIR/sqlitegis.dylib"

# Check if extension exists
if [ ! -f "$EXTENSION_LIB" ]; then
    echo -e "${RED}Error: Extension library not found at $EXTENSION_LIB${NC}"
    echo "Please build the extension first:"
    echo "  cd $WORKSPACE_DIR"
    echo "  cmake --build build"
    exit 1
fi

# Check if SQLite3 supports loadable extensions
if ! sqlite3 --version &> /dev/null; then
    echo -e "${RED}Error: sqlite3 command not found${NC}"
    exit 1
fi

# Test if extensions can be loaded
TEST_LOAD=$(sqlite3 :memory: "SELECT load_extension('$EXTENSION_LIB');" 2>&1 || true)
if echo "$TEST_LOAD" | grep -q "not authorized"; then
    echo -e "${RED}Error: SQLite3 does not support loadable extensions${NC}"
    echo "You may need to:"
    echo "  1. Build SQLite from source with --enable-load-extension"
    echo "  2. Use the SQLite3 binary from third_party/sqlite-install/bin/sqlite3"
    exit 1
fi

echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║           SqliteGIS Extension Test Suite                      ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Extension: $EXTENSION_LIB"
echo "SQLite3:   $(sqlite3 --version)"
echo ""

# Test files to run (in order)
TEST_FILES=(
    "test_constructors.sql"
    "test_accessors.sql"
    "test_measures.sql"
    "test_relations.sql"
    "test_operations.sql"
)

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Run each test file
for test_file in "${TEST_FILES[@]}"; do
    test_path="$TESTS_DIR/$test_file"
    
    if [ ! -f "$test_path" ]; then
        echo -e "${YELLOW}Warning: Test file not found: $test_file${NC}"
        continue
    fi
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Running: $test_file${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
    
    # Run the test (redirect to temp file for error checking)
    TEMP_OUTPUT=$(mktemp)
    if sqlite3 :memory: < "$test_path" > "$TEMP_OUTPUT" 2>&1; then
        cat "$TEMP_OUTPUT"
        echo ""
        echo -e "${GREEN}✓ $test_file completed${NC}"
        ((PASSED_TESTS++))
    else
        cat "$TEMP_OUTPUT"
        echo ""
        echo -e "${RED}✗ $test_file failed${NC}"
        ((FAILED_TESTS++))
    fi
    
    rm -f "$TEMP_OUTPUT"
    ((TOTAL_TESTS++))
    echo ""
done

# Summary
echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                       Test Summary                             ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Total Test Suites: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
else
    echo -e "${GREEN}Failed: 0${NC}"
fi
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                 All Tests Passed! 🎉                           ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════════╝${NC}"
    exit 0
else
    echo -e "${RED}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║              Some Tests Failed ❌                              ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════════╝${NC}"
    exit 1
fi
