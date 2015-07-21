// Make Chinese dictionary for sphinx_chinese
// 2009 blueflycn
// All rights reserved.

#include "darts-clone.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

int progress_bar(size_t current, size_t total)
{
	static const char bar[] = "******************************";
	static const int scale = sizeof(bar) - 1;

	static int prev = 0;

	int cur_percentage = static_cast<int>(100.0 * current / total);
	int bar_len = static_cast<int>(1.0 * current * scale / total);

	if (prev != cur_percentage)
	{
		printf("Making Chinese dictionary:\t%3d%% |%.*s%*s|",
			cur_percentage, bar_len, bar, scale - bar_len, "");
		if (cur_percentage >= 100)
			printf("\n");
		else
			printf("\r");
		fflush(stdout);
	}

	prev = cur_percentage;

	return 1;
};

struct ltstr
{
  bool operator()(string s1, string s2) const
  {
    return s1<s2;
  }
};


int input_keys(istream &key_input_stream,
	vector<char> &keys_buf, vector<const char *> &keys, vector<int> &values)
{
  map<string, int, ltstr> dict;
	string key_string;
	vector<size_t> indices;
	char * pEnd;
	while (getline(key_input_stream, key_string))
	{
		dict.insert ( pair<string,int>(key_string.substr(0,key_string.find('\t',0)),strtol(key_string.substr(key_string.find('\t',0)+1,key_string.size()-1).c_str(),&pEnd,0)) );
	}
	for(map<string, int, ltstr>::iterator cur  = dict.begin();cur!=dict.end();cur++){
		indices.push_back(keys_buf.size());
		keys_buf.insert(keys_buf.end(), cur->first.begin(), cur->first.end());
		keys_buf.push_back('\0');
		values.push_back(cur->second);
	}

	vector<char>(keys_buf).swap(keys_buf);
	keys.resize(indices.size());
	for (size_t i = 0; i < indices.size(); ++i) {
		keys[i] = &keys_buf[indices[i]];
	}
	return dict.size();

}

int build_da(istream &key_input_stream,
	const string &index_file_path)
{
	vector<char> keys_buf;
	vector<const char *> keys;
	vector<int> values;
	int count;

	cout<<"Preparing..."<<endl;
	cout.flush();
	count=input_keys(key_input_stream, keys_buf, keys, values);

	Darts::DoubleArray da;
		if (da.build(keys.size(), &keys[0], 0, &values[0], progress_bar) != 0)
		{
			cerr << "\nError: cannot build Chinese dictionary" << endl;
			return 1;
		}

	if (da.save(index_file_path.c_str()) != 0)
	{
		cerr << "Error: cannot save Chinese dictionary: " << index_file_path << endl;
		return 1;
	}
	cout << "Total words:\t\t\t"<<count<<endl;
	cout << "File size:\t\t\t"<<da.size()*4<<" bytes"<<endl;
	cout << "Compression ratio:\t\t" <<
	    100.0 * da.nonzero_size() / da.size() << " %" << std::endl;
	cout << "Chinese dictionary was successfully created!" << std::endl;
	return 0;
}

int mkdarts(istream &key_input_stream,
	const string &index_file_path)
{
	try
	{
		return build_da(key_input_stream, index_file_path);
	}
	catch (const std::exception &ex)
	{
		cerr << "Error: " << ex.what() << endl;
	}

	return 1;
}

void show_usage(const char *cmd)
{
	cerr << "Usage: " << cmd << " wordsfile dictfile" << endl;
	cerr << "Make Chinese dictionary from wordsfile" << endl;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		show_usage(argv[0]);
		return 1;
	}


	const string key_file_path(argv[1]);
	const string index_file_path(argv[2]);

	if (key_file_path == "-")
		return mkdarts(cin, index_file_path);

	ifstream key_file(key_file_path.c_str());
	if (!key_file.is_open())
	{
		cerr << "Error: cannot open: " << key_file_path << endl;
		return 1;
	}

	return mkdarts(key_file, index_file_path);
}
