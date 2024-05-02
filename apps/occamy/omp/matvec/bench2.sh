
outputt=matvec_output
mkdir -p $outputt


for i in $(seq 10)
do
    for j in $(seq 5)
    do
        size=$(( j * 128 ))
        echo "$size-$i"
        ./matvec $size > out.txt
        lines_=$(wc out.txt)
        echo $lines_
        lines=$(echo $lines_|cut -d' ' -f1)
        echo $lines
        if [ "$lines" -gt 5 ]; then
            echo "Copying"
            head -n -1 out.txt > $outputt/$size-$i.txt
        fi
    done
done
