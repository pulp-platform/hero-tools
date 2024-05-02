mkdir -p helloworld_output
for i in $(seq 10)
do
    echo $i
    ./helloworld > out.txt
    lines_=$(wc out.txt)
    echo $lines_
    lines=$(echo $lines_|cut -d' ' -f1)
    echo $lines
    if [ "$lines" -gt 5 ]; then
        echo "Copying"
        head -n -1 out.txt > helloworld_output/$i.txt
    fi 
done
