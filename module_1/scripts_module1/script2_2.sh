touch "practice2_${MYUSER}_part2.log"
BOOT=$(date -d "$(uptime -s)" '+%Y-%m-%d %H:%M:%S')
END=$(date -d "$BOOT 20 seconds" '+%Y-%m-%d %H:%M:%S')
journalctl -b --since "$BOOT" --until "$END" > "20_sec_journalctl.log"
find /var/log -type f -name 'auth.log' 2>/dev/null
awk '{source=$3; message=""; for (i=4; i<=NF; i++) message=message (i==4 ? "" : " ") $i; print source, message}' /var/log/auth.log | sed -E 's/\[[0-9]+\]//' | sort -k1,1 > clean_auth.log
awk '{print $1}' clean_auth.log | sed -E 's/\[[0-9]+\]//' | sort | uniq -c | sort -rn > log_count.log
cat /etc/passwd |wc -l
awk -F: '$7=="/usr/sbin/nologin" {print $3, $1}' /etc/passwd | sort -rn |wc -l
while IFS=: read -r username password uid gid gecos home shell; do
    if [ "$shell" = "/usr/sbin/nologin" ]; then
        groups_list=$(awk -F: -v u="$username" -v g="$gid" \
            '$3==g || $4 ~ ("(^|,)" u "(,|$)") {print $1}' /etc/group |
            paste -sd, -)
        printf '%s, %s, %s\n' "$uid" "$username" "$groups_list"
    fi
done < /etc/passwd | sort -t, -k1,1nr
dmesg | grep 'kernel'
wc -l /var/log/kern.log
awk '/^Start-Date:/ {start=$0} /^Commandline:/ {cmd=$0} /^End-Date:/ {print cmd " ; " start " ; " $0}' /var/log/apt/history.log > formatted_history.log