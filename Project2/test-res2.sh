if [ ! -e p3150258-p3150106-pizza2.c ]; then
    echo "file p3150258-p3150106-pizza2.c not found"
    exit 1
fi

gcc -o main2 p3150258-p3150106-pizza2.c -pthread

echo -e "Running with seed: 1000 and 100 customers ..."
./main2 100 1000
