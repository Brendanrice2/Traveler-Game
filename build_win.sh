# This script is used to build the project on WSL
echo "Building..."
g++ -Wall -std=c++20 ./Programs/Version2/*.cpp -lGL -lglut -o final2 -pthread
echo "Done!"
./final 10 15 2 0