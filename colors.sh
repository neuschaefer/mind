#/bin/sh
for i in 0 1; do
	for j in `seq 0 7`; do
		printf "\e[$i;3${j}m 0$i$j"
#		printf "\033[m"
	done; echo
done; printf "\033[m"
