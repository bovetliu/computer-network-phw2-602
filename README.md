# phw2-602

to test, make sure computer have tftp (if not, sudo apt-get install tftp)

I used codeblock to compile, 

run server:

./bin/Debug/jiahao_hw2 127.0.0.1 3000

One can put one test file(myfile.txt), say, in folder ~/tmp/
Then run command:

tftp 127.0.0.1 3000

tftp> get ~/tmp/myfile.txt

then one new myfile.txt will be saved under ~/ folder 
