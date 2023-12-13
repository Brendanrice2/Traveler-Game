echo "Building..."
g++ -Wno-deprecated -std=c++20 -Wall ./Programs/Version1/*.cpp -framework OpenGL -framework GLUT -o prog1
echo "Done!"