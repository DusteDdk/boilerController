Requires:
SDL2
jq

Building:
bash build.txt

Running:
export LC_NUMERIC="en_US.UTF-8"; nc 192.168.20.40 9000 | while read I; do printf "%03.1f\\n" `echo $I | jq -r .temp`  ; done | ./grapher
