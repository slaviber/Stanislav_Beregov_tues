#include <string>
#include <vector>
#include <iostream>
#include <istream>
#include <ostream>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <utility>
using namespace std;

string types[9] = {"varchar", "date", "decimal", "longtext", "double", "int", "float", "bit", "char"};
const int ONE_RELATED = 0, CONTAINED_BY = 1, CONTAINS = 2;

string parse_var(string var){
	if(var == "varchar")return "varchar(255)";
	if(var == "currency")return "decimal(5,2)";
	if(var == "string")return "varchar(255)";
	if(var == "bool")return "bit(1)";
	int type = var.find('(');
	string check = var.substr(0, type);
	bool flag = false;
	for(int i = 0; i < 9; ++i){
		if(types[i] == check)flag = true;
	}
	if(!flag)return "ERR";
	return var;
}

struct Table{
	string name;
	vector<string> coltypes;
	vector<string> colnames;
	vector<string> ones;
};

int find_table(string name, vector<Table>& tbuf){
		for(int i = 0; i < tbuf.size(); ++i){
			if(tbuf[i].name == name){
				return i;
			}
		}
		return -1;
}

bool add_new(vector<string> results, vector<string> &buffer, vector<Table>& tbuf){
	Table t;
	string s;
	if(find_table(results[1], tbuf) != -1)return false;
	t.name = results[1];
	s += "CREATE TABLE " + results[1] + "(id int auto_increment primary key";
	for(int i = 2; i < results.size(); i += 2){
		string res = parse_var(results[i+1]);
		if(res == "ERR"){
			return false;
		}
		t.coltypes.push_back(res);
		t.colnames.push_back(results[i]);
		s += ", " + results[i] + " " + res;
	}
	s += ");";
	buffer.push_back(s);
	tbuf.push_back(t);
	return true;
}

void alter_table(int table, string other_name, vector<string> &buffer, vector<Table>& tbuf){
		tbuf[table].coltypes.push_back("int");
		tbuf[table].colnames.push_back(other_name + "_id");
		string alter = "ALTER TABLE ";
		alter += tbuf[table].name;
		alter += " ADD COLUMN ";
		alter += other_name + "_id INT;";
		buffer.push_back(alter);
}

bool add_relation(vector<string> results, vector<string> &buffer, vector<Table>& tbuf){
	int ind1 = find_table(results[2], tbuf);
	int ind2 = find_table(results[3], tbuf);
	if(ind1 == -1 || ind2 == -1) return 0;
	if(results[1] == "mm"){
		vector<string> arg;
		arg.push_back("create");
		arg.push_back(results[2] + "_" + results[3]);
		arg.push_back(results[2] + "_id");
		arg.push_back("int");
		arg.push_back(results[3] + "_id");
		arg.push_back("int");
		if(!add_new(arg, buffer, tbuf))return false;
	}
	else if(results[1] == "oo"){
		tbuf[ind1].ones.push_back(results[3] + "_id");
		tbuf[ind2].ones.push_back(results[2] + "_id");
	}
	else if(results[1] == "om"){
		alter_table(ind2, results[2], buffer, tbuf);
	}
	else if(results[1] == "mo"){
		alter_table(ind1, results[3], buffer, tbuf);
	}
	else return 0;
	return 1;
}

void write(vector<string> results, vector<string>& buffer){
	ofstream out(results[1].c_str());
	for(int i = 0; i < buffer.size(); ++i){
		out << buffer[i] << endl;
	}
	buffer.clear();
	out.close();
}

bool insert_values(vector<string> results, vector<string> &buffer, vector<Table>& tbuf){
	string query;
	int inc = 0;
	for(int j = 0; j < tbuf.size(); ++j){
		query = "INSERT INTO ";
		query += tbuf[j].name;
		query += " (";
		for(int k = 0; k < tbuf[j].colnames.size(); ++k){
			query += tbuf[j].colnames[k] + ", ";
		}
		query = query.substr(0, query.length()-2);
		query +=  ") values ";
		for(int i = 0; i < atoi(results[1].c_str()); ++i){
			query += "(";
			for(int k = 0; k < tbuf[j].coltypes.size(); ++k){
				int del = tbuf[j].coltypes[k].find('(');
				string def = tbuf[j].coltypes[k].substr(0, del);
				string c = " ";
				c[0] = '0' + inc;
				++inc;
				inc %= 10;
				if(def  == "int" || def  == "float" || def  == "double" || def  == "decimal"){
					query += c + ", ";
				}
				else if (def  == "varchar" || def  == "longtext"){
					string s = tbuf[j].colnames[k] + c;
					query += "'" + s + "', ";
				}
				else if(def  == "date"){
					if(c == "0")c = "10";
					query += "'2015-04-" + c + "', ";
				}
				else if(def == "bit"){
					query += inc%2 ? "false, " : "true, ";
				}
				else if(def == "char"){
					string m = " ";
					m[0] = 'a' + inc;
					query += "'"  + m + "', ";
				}
				else return false;
			}
			query = query.substr(0, query.length()-2);
			query += "), ";
		}
		query = query.substr(0, query.length()-2);
		query += ";";
		buffer.push_back(query);
	}
	return true;
}

vector<pair<string, int> > find_relation(vector<Table>& tbuf, string curr, string end, string back){ 
	string name = curr + "_id";
	vector<pair<string, int> > res;
	if(curr == end){
		res.push_back(make_pair(end, -1));
		return res;
	}
	int curr_ind = -1;
	vector<string> names;
	for(int i = 0; i < tbuf.size(); ++i){
		if(tbuf[i].name == curr){
			curr_ind = i;
			continue;
		}
		if(tbuf[i].name != back)names.push_back(tbuf[i].name);
		for(int j = 0; j < tbuf[i].colnames.size(); ++j){
			if(tbuf[i].colnames[j] == name){
				vector<pair<string, int> > s = find_relation(tbuf, tbuf[i].name, end, curr);
				if(!s.empty()){
					s.push_back(make_pair(curr, CONTAINED_BY));
					return s;
				}
			}
		}
		for(int j = 0; j < tbuf[i].ones.size(); ++j){
			if(tbuf[i].ones[j] == name){
				vector<pair<string, int> > s = find_relation(tbuf, tbuf[i].name, end, curr);
				if(!s.empty()){
					s.push_back(make_pair(curr, ONE_RELATED));
					return s;
				}
			}
		}
	}
	if(curr_ind == -1){
		return res;
	}
	for(int i = 0; i < tbuf[curr_ind].colnames.size(); ++i){
		for(int a = 0; a < names.size(); ++a){
			if(tbuf[curr_ind].colnames[i] == (names[a] + "_id")){
				vector<pair<string, int> > s = find_relation(tbuf, names[a], end, curr);
				if(!s.empty()){
					s.push_back(make_pair(curr, CONTAINS));
					return s;
				}
			}
		}
	}
	return res;
}

bool do_select(vector<string> results, vector<string> &buffer, vector<Table>& tbuf){
	vector<pair<string, int> > s = find_relation(tbuf, results[1], results[2], "");
	if(s.empty())return false;
	string query = "SELECT * FROM ";
	query += s[s.size()-1].first;
	for(int i = s.size()-2; i >= 0; --i){
		query += " JOIN " + s[i].first + " ON ";
		int state = s[i+1].second;
		switch(state){
			case ONE_RELATED:
				query += s[i+1].first + ".id = " + s[i].first + ".id"; 
				break;
			case CONTAINED_BY:
				query += s[i+1].first + ".id = " + s[i].first + "." + s[i+1].first + "_id";
				break;
			case CONTAINS:
				query += s[i+1].first + "." + s[i].first + "_id = " + s[i].first + ".id";
				break;
		}
	}
	query += " WHERE " + s[0].first + ".id = 1;";
	buffer.push_back(query);
	return true;
}

bool migrate(vector<string> results, vector<string> &buffer, vector<Table>& tbuf){
	int id = -1;
	for(int i = 0; i < tbuf.size(); ++i){
		if(results[1] == tbuf[i].name){
			id = i;
			break;
		}
	}
	if(id == -1)return false;
	vector<string> fields_1;
	for(int i = 4; i < results.size(); ++i){
		if(find(tbuf[id].colnames.begin(), tbuf[id].colnames.end(), results[i])!=tbuf[id].colnames.end()){
			fields_1.push_back(results[i]);
		}
		else return false;
	}
	vector<string> arg2;
	arg2.push_back("create");
	arg2.push_back(results[2]);
	vector<string> arg;
	arg.push_back("create");
	arg.push_back(results[3]);
	for(int i = 0; i < tbuf[id].colnames.size(); ++i){
		if(find(fields_1.begin(), fields_1.end(), tbuf[id].colnames[i])!=fields_1.end()){
			arg2.push_back(tbuf[id].colnames[i]);
			arg2.push_back(tbuf[id].coltypes[i]);
		}
		else{
			arg.push_back(tbuf[id].colnames[i]);
			arg.push_back(tbuf[id].coltypes[i]);
		}
	}
	if(!add_new(arg2, buffer, tbuf))return false;
	if(!add_new(arg, buffer, tbuf))return false;
	return true;
}

int main(){
	vector<string> buffer;
	vector<Table> tbuf;
	while(1){
		begin:
		string query;
		 getline(cin, query);

		stringstream strstr(query);
		istream_iterator<string> it(strstr);
		istream_iterator<string> end;
		vector<string> results(it, end);
	
		if(results.size() > 0){
			if(results[0] == "create" && results.size() >= 2){
				//create table_name []nameN typeN]
				if(!add_new(results, buffer, tbuf))cout << "ERR" << endl;
			}
			if(results[0] == "write" && results.size() == 2){
				//write filename
				write(results, buffer);
			}
			if(results[0] == "relate" && results.size() == 4){
				//relate om|mo|oo|mm table1 table2
				if(!add_relation(results, buffer, tbuf))cout << "ERR" << endl;
			}
			if(results[0] == "insert" && results.size() == 2){
				//insert N
				if(!insert_values(results, buffer, tbuf))cout << "ERR" << endl;
			}
			if(results[0] == "select" && results.size() == 3){
				//select table1 for table2
				if(!do_select(results, buffer, tbuf))cout << "ERR" << endl;
			}
			if(results[0] == "migrate" && results.size() >= 4){
				//migrate table new_name1 new_name2 [new1_fields]
				if(!migrate(results, buffer, tbuf))cout << "ERR" << endl;
			}
		}
		cout << endl;
	}
return 0;
}
