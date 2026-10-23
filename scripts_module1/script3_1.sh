su - user1
ps -eo nlwp,pid,args | awk '$1 > 1'
top #f,s,q,space_bar,up,down,right,left,enter
passwd
top
kill -15 PID
kill -2 PID
kill -3 PID
kill -9 PID
top 
vim ~/file_task3.txt
jobs
sleep 600
bg %2
jobs
pgrep -a sleep
renice 10 -p PID
ps -p PID -o pid,ni,cmd
fg %1
kill -TERM %2
jobs
trap 'echo "Меня голыми руками не возьмёшь!"' SIGINT SIGQUIT
trap -p SIGINT SIGQUIT
kill -SIGINT "$$"
kill -SIGQUIT "$$"
trap - SIGINT SIGQUIT
(sleep 1 & exec /bin/sleep 30) &
(sleep 1 & exec /bin/sleep 30) &
(sleep 1 & exec /bin/sleep 30) &
ps -eo pid,ppid,stat,comm | awk '$3 ~ /Z/ {print "PID=" $1, "PPID=" $2, "STATE=" $3, "COMMAND=" $4}'
