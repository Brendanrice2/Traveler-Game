# This script is used to build the project on WSL
echo "Building..."
g++ -Wall -std=c++20 ./Programs/Code/*.cpp ./Programs/Version1/*.cpp -lGL -lglut -o final -pthread
echo "Done!"
#./final 30 35 1 0
./final 10 15 1 0