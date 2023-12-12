# This script is used to build the project on WSL

g++ -Wall -std=c++20 ./Programs/Code/*.cpp ./Programs/Version1/*.cpp -lGL -lglut -o final -pthread

./final 30 35 1 0