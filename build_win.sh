# This script is used to build the project on WSL
echo "Building..."
g++ -Wall -std=c++20 ./Programs/Version1/*.cpp -lGL -lglut -o final -pthread
echo "Done!"
./final 10 15 1 0