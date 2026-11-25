
#!/bin/bash

set -u

echo "========== BUILD =========="

make clean
make || exit 1

mkdir -p test_data
cd test_data || exit 1

echo "Hello, World!" > file1.txt
echo "This is another file." > file2.txt
: > empty.txt

echo
echo "========== TEST 1: Anonymous pipe =========="

../copy file1.txt

if cmp -s file1.txt file1.txt.copy; then
    echo "PASS: file1 copied correctly"
else
    echo "FAIL: file1 differs"
fi

echo
echo "========== TEST 2: Multiple files =========="

../copy file1.txt file2.txt empty.txt

for file in file1.txt file2.txt empty.txt; do
    if cmp -s "$file" "$file.copy"; then
        echo "PASS: $file"
    else
        echo "FAIL: $file"
    fi
done

echo
echo "========== TEST 3: Named FIFO =========="

../copy -p testpipe file1.txt file2.txt

for file in file1.txt file2.txt; do
    if cmp -s "$file" "$file.copy"; then
        echo "PASS: $file via FIFO"
    else
        echo "FAIL: $file via FIFO"
    fi
done

echo
echo "========== TEST 4: Missing file =========="

../copy nonexistent.txt 2> error.log

if grep -q "nonexistent.txt" error.log; then
    echo "PASS: error written to stderr"
else
    echo "FAIL: error not detected"
fi

echo
echo "========== TEST 5: No arguments =========="

if ../copy 2> error.log; then
    echo "FAIL: expected an error"
else
    echo "PASS: invalid arguments detected"
fi

echo
echo "========== TEST FINISHED =========="
