#include "../threadPool/threadPool.h"
#include <iostream>
#include <random>
#include <string>
using namespace std;
random_device rd;
mt19937 mt(rd());
uniform_int_distribution<int> dist(-1000, 1000);
auto rnd = bind(dist, mt);
void sleep() {
	this_thread::sleep_for(chrono::milliseconds(2000 + rnd()));
}
int mut(const int a, const int b) {
	sleep();
	return a * b;
}
string hello(const string& a, const string& b) {
	return a + b;
}
int main() {
	threadPool tp;
	tp.init();
	future<int> f[5];
	future<string> f2[5];
	for (int i = 0; i < 5; ++i) {
		f[i] = tp.submit(mut, i, i + 1);
	}
	for (int i = 0; i < 5; ++i) {
		f2[i] = tp.submit(hello, to_string(i), to_string(i + 1));
	}
	for (int i = 0; i < 5; ++i) {
		cout << f[i].get() << endl;
	}
	for (int i = 0; i < 5; ++i) {
		cout << f2[i].get() << endl;
	}
	tp.shotdown();
}