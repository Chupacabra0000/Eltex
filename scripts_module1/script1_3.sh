cd ~

touch "${USER}_part3.log"

mkdir -p D1/D2/D3

touch D1/D2/file.txt

echo "Hello from file.txt" > D1/D2/file.txt

cd D1/D2/D3

ln -s ../file.txt symbolic_link
ln ../file.txt hard_link

ls -li ../file.txt symbolic_link hard_link

cd ~

mv D1/D2/file.txt D1/

cat D1/D2/D3/hard_link
cat D1/D2/D3/symbolic_link

rm -rf D1/D2

find / -type f -size +20M 2>/dev/null \
    > "${USER}_part3_gt20M.log"

cat "${USER}_part3_gt20M.log"

while read file; do
    du -h "$file"
done < "${USER}_part3_gt20M.log"

find ~ -type f -mtime -1

which find

file "$(which find)"

file /boot/initrd.img*