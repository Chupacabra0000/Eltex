#На сервере 1:
ssh-keygen -t ed25519
ls -l ~/.ssh/*.pub
ssh-copy-id -i ~/.ssh/id_ed25519.pub root@172.16.9.221
ssh root@172.16.9.221
cat ~/.ssh/id_ed25519.pub
ls -l ~/.ssh/id_ed25519.pub

#На сервере 2:
$ export MYUSER="eltex-pg2-v21"
$ touch "practice2_${MYUSER}_part1.log"

$ cat ~/.ssh/authorized_keys
$ ls -ld ~/.ssh
$ ls -l ~/.ssh/authorized_keys

$ useradd -m -s /bin/bash user1
$ passwd user1
$ useradd -m -s /bin/bash user2
$ passwd user2
$ useradd -m -s /bin/bash user3
$ passwd user3

$ id user1
$ id user2
$ id user3

$ getent passwd user1
$ getent passwd user2
$ getent passwd user3


$ usermod -s /usr/sbin/nologin user3
getent passwd user3


$ su - user3
This account is currently not available.

$ su -s /bin/bash user3
user3@eltex-practice2-pg2-v21:/root$ exit
exit

$ groupadd practicegroup
$ usermod -aG practicegroup user1
$ usermod -aG practicegroup user2
$ usermod -aG practicegroup user3

$ id user1
$ id user2
$ id user3

$ getent group practicegroup
$ mkdir -p /opt/share
$ chown root:practicegroup /opt/share
$ chmod u=rwx,g=rws,o= /opt/share
$ ls -ld /opt/share

$ echo 'umask 027' >> /home/user1/.bashrc
$ su - user1
$$ umask
0027
$$ exit
logout

$ chmod 2770 /opt/share
$ su - user1 -c 'echo user1 > /opt/share/user1.txt'
$ su - user2 -c 'echo user2 > /opt/share/user2.txt'
$ su - user3 -c 'echo user3 > /opt/share/user3.txt'
This account is currently not available.

$ su - user1 -c 'rm /opt/share/user2.txt'

$ chmod 3770 /opt/share
$ su - user1 -c 'echo user1 > /opt/share/user1.txt'
$ su - user2 -c 'echo user2 > /opt/share/user2.txt'
$ su - user3 -c 'echo user3 > /opt/share/user3.txt'
This account is currently not available.

$ su - user1 -c 'rm /opt/share/user2.txt'
rm: cannot remove '/opt/share/user2.txt': Operation not permitted

$ ls -ld /opt/share
drwxrws--T 2 root practicegroup 4096 Oct 19 10:40 /opt/share

$ command -v sudo
/usr/bin/sudo

$ visudo -f /etc/sudoers.d/user1-dmesg
$ cat /etc/sudoers.d/user1-dmesg
user1 ALL=(root) /usr/bin/dmesg

$$ sudo -l
Matching Defaults entries for user1 on eltex-practice2-pg2-v21:
    env_reset, mail_badpass, secure_path=/usr/local/sbin\:/usr/local/bin\:/usr/sbin\:/usr/bin\:/sbin\:/bin\:/snap/bin, use_pty

User user1 may run the following commands on eltex-practice2-pg2-v21:
    (root) /usr/bin/dmesg
$ su - user1
$$ sudo dmesg
$$ logout

$ visudo -f /etc/sudoers.d/user2-dmesg
$  cat /etc/sudoers.d/user2-dmesg

$ su - user2
$$ sudo -l
$$ touch script.sh dmesg.sh
$$ chmod u+s script.sh dmesg.sh
$$ cat script.sh dmesg.sh
$$ sudo ./script.sh
$$ bash dmesg.sh
$$ logout

$ chage -M 10 user1
$ chage -M 10 user2
$ chage -M 10 user3

$ chage -l user1
$ chage -l user2
$ chage -l user3

$ echo 'Shoaib Shahid' >> /etc/motd
$ date '+%Y-%m-%d %H:%M:%S' >> /etc/motd
$ cat /etc/motd

$ mkdir -p /root/etc_backup

$ rsync -a /etc/ /root/etc_backup/
$ rsync -avn /etc/ /root/etc_backup/

$ tar -czf etc_backup.tar.gz etc_backup/
$ tar -cjf etc_backup.tar.bz2 etc_backup/
$ tar -caf etc_backup.7z etc_backup/

$ ls -lh etc_backup.tar.gz etc_backup.tar.bz2 etc_backup.7z

$ date '+%Y-%m-%d %H:%M:%S' >> /etc/motd
$ cat /etc/motd

$ rsync -a /etc/ /root/etc_backup/
$ tar -rzf /root/etc_backup.tar.gz -C /root/etc_backup etc/motd
$ tar -uzf /root/etc_backup.tar.gz -C /root/etc_backup etc/motd
$ tar -czf /root/etc_backup_new.tar.gz -C /root etc_backup/
$ tar -df etc_backup.tar.bz2

$ mkdir -p /root/etc_backup_gzip
$ mkdir -p /root/etc_backup_7zip

$ tar -xzf /root/etc_backup.tar.gz -C /root/etc_backup_gzip
$ tar -axf /root/etc_backup.7z -C /root/etc_backup_7zip
$ diff -s /root/etc_backup_gzip/etc_backup/motd /root/etc_backup_7zip/etc_backup/motd

$ mkdir -p new_gzip
$ tar -xzf /root/etc_backup_new.tar.gz -C /root/new_gzip
$ diff -s  /root/new_gzip/etc_backup/motd /root/etc_backup_gzip/etc_backup/motd
3d2
< 2026-10-21 06:38:59







