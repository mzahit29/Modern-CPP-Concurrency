// ConcurrencyProject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "Examples.h"

using namespace std;

int main()
{
	cout << "Main thread start" << endl;


	Examples::join_and_detach_run();
	Examples::handling_join_in_exceptions_run();

	system("pause");
	cout << "Main thread end" << endl;
    return 0;
}

