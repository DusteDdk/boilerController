Requires:
SDL2
jq

Building:
bash build.txt

Running:
nc 192.168.20.109 9000 | while read I; do printf "%03.1f\\n" `echo $I | jq -r .temp`  ; done | ./grapher
