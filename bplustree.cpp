#include <iostream>
#include <vector>
#include <fstream>
#include <string>
using namespace std;

#define ISLEAF (true)

class Node{
public:
	bool isLeaf;
	vector<int> key;
	vector<Node *> child;
	vector<double> value;
	Node *parent = NULL;
	// doubly linked list
	Node *prev = NULL, *next = NULL;
	void init(bool isLeaf, int m){
		this->isLeaf = isLeaf;
	}
	// add to linked list
	void addNext(Node *node){
		if(!this->next){
			this->next = node;
			node->prev = this;
			return;
		}
		Node *next = this->next;
		next->prev = node;
		this->next = node;
		node->next = next;
		node->prev = this;
		return;
	}
	void remove(){
		if(this->next){
			this->next->prev = this->prev;
		}
		if(this->prev){
			this->prev->next = this->next;
		}
	}
};

class BPlusTree{
private:
	int m;
	Node *root = NULL;

	Node *_insertToLeaf(Node *node, int k, double v){
		while(!node->isLeaf){
			int i;
			for(i = 0; i < node->key.size(); i++){
				if(k < node->key[i]) break;
			}
			node = node->child[i];
		}
		// insert into an vector
		for(int i = 0; i < node->key.size(); i++){
			if(k < node->key[i]) {
				node->key.insert(node->key.begin()+i, k);
				node->value.insert(node->value.begin()+i, v);
				break;
			}
			if(i == node->key.size()-1){
				node->key.push_back(k);
				node->value.push_back(v);
				break;
			}
		}
		return (node->key.size() < m)? root: _splitNodeAndMerge(node); 
	}
	Node *_splitNodeAndMerge(Node *node){
		int key = node->key[m/2];
		// split node
		Node *new_child = new Node();
		if(node->isLeaf){
			new_child->init(ISLEAF, m);
			new_child->key.insert(new_child->key.begin(), node->key.begin()+m/2, node->key.end());
			new_child->value.insert(new_child->value.begin(), node->value.begin()+m/2, node->value.end());
			node->key.erase(node->key.begin()+m/2, node->key.end());
			node->value.erase(node->value.begin()+m/2, node->value.end());
		}
		else{
			new_child->init(!ISLEAF, m);
			new_child->key.insert(new_child->key.begin(), node->key.begin()+m/2+1, node->key.end());
			new_child->child.insert(new_child->child.begin(), node->child.begin()+m/2+1, node->child.end());
			for(auto it : new_child->child){
				it->parent = new_child;
			}
			node->key.erase(node->key.begin()+m/2, node->key.end());
			node->child.erase(node->child.begin()+m/2+1, node->child.end());
		}
		node->addNext(new_child);


		// Merge
		Node *parent = node->parent;
		// Make new node and become new root
		if(!parent){
			Node *new_node = new Node();
			new_node->init(!ISLEAF, m);
			new_node->key.push_back(key);
			new_node->child.push_back(node);
			new_node->child.push_back(new_child);
			node->parent = new_node;
			new_child->parent = new_node;
			return new_node;
		}
		// add new node to parent node
		new_child->parent = parent;
		for(int i = 0; i < parent->key.size(); i++){
			if(key < parent->key[i]){
				parent->key.insert(parent->key.begin()+i, key);
				parent->child.insert(parent->child.begin()+i+1, new_child);
				break;
			}
			if(i == parent->key.size()-1){
				parent->key.push_back(key);
				parent->child.push_back(new_child);
				break;
			}
		}
		return (parent->key.size() < m)? root: _splitNodeAndMerge(parent);
	}
	int _getChildIdxFromParent(Node *node){
		Node *parent = node->parent;
		for(int i = 0; parent && i < parent->child.size(); i++){
			if(node == parent->child[i]) 
				return i;
		}
		return -1;
	}
	Node *_fixDeficientNode(Node *node){
		Node *parent = node->parent;
		// node is root and deficient
		if(!parent){
			if(node->isLeaf && node->value.size() == 0) {
				delete node;
				return NULL;
			}
			Node *new_root = node->child[0];
			new_root->parent = NULL;
			delete node;
			return new_root;
		}
		int idx = _getChildIdxFromParent(node);
		if(_borrowFromSibling(node, idx)) return root;
		// merge with prev
		if(idx > 0){
			if(!node->isLeaf)
				node->key.push_back(parent->key[idx-1]);
			_mergeTwoNode(parent->child[idx-1], node);
			parent->child.erase(parent->child.begin()+idx);
			parent->key.erase(parent->key.begin()+(idx-1));
		}
		// merge with next
		else{
			if(!node->isLeaf)
				node->key.push_back(parent->key[idx]);
			_mergeTwoNode(node, parent->child[idx+1]);
			parent->child.erase(parent->child.begin()+idx+1);
			parent->key.erase(parent->key.begin()+idx);
		}
		return (parent->key.size() > 0)? root: _fixDeficientNode(parent);
	}
	Node *_mergeTwoNode(Node *left, Node *right){	
		left->key.insert(left->key.end(), right->key.begin(), right->key.end());
		left->child.insert(left->child.end(), right->child.begin(), right->child.end());
		left->value.insert(left->value.end(), right->value.begin(), right->value.end());
		right->remove();
		delete right;

		return left;
	}
	bool _borrowFromSibling(Node *node, int idx){
		return (node->isLeaf)? _borrowFromSiblingLeaf(node, idx)
		: _borrowFromSiblingNode(node, idx);
	}
	bool _borrowFromSiblingLeaf(Node *node, int idx){
		Node *sib = node->prev;
		// borrow half from prev
		if(sib && sib->parent == node->parent && sib->key.size() > 1){
			int sib_size = sib->key.size();
			int new_key = sib->key[sib_size/2];
			node->key.insert(node->key.begin(), sib->key.begin()+sib_size/2, sib->key.end());
			node->value.insert(node->value.begin(), sib->value.begin()+sib_size/2, sib->value.end());
			sib->key.erase(sib->key.begin()+sib_size/2, sib->key.end());
			sib->value.erase(sib->value.begin()+sib_size/2, sib->value.end());
			node->parent->key[idx-1] = new_key;
			return true;
		}
		sib = node->next;
		// borrow half from next
		if(sib && sib->parent == node->parent && sib->key.size() > 1){
			int sib_size = sib->key.size();
			int new_key = sib->key[sib_size/2];
			node->key.insert(node->key.begin(), sib->key.begin(), sib->key.begin()+(sib_size/2));
			node->value.insert(node->value.begin(), sib->value.begin(), sib->value.begin()+(sib_size/2));
			sib->key.erase(sib->key.begin(), sib->key.begin()+(sib_size/2));
			sib->value.erase(sib->value.begin(), sib->value.begin()+(sib_size/2));
			node->parent->key[idx] = new_key;
			return true;		
		}
		// borrow failed
		return false;
	}
	bool _borrowFromSiblingNode(Node *node, int idx){
		Node *parent = node->parent;

		Node *prev = NULL, *next = NULL;
		if(idx > 0) prev = node->prev;
		if(idx < parent->child.size()-1) next = node->next;

		// borrow one from prev
		if(prev && prev->key.size() > 1){
			node->key.push_back(parent->key[idx-1]);
			parent->key[idx-1] = prev->key.back();
			node->child.insert(node->child.begin(), prev->child.back());
			node->child[0]->parent = node;
			prev->child.pop_back();
			prev->key.pop_back();
			return true;
		}
		// borrow one from next
		if(next && next->key.size() > 1){
			node->key.push_back(parent->key[idx]);
			parent->key[idx] = next->key.front();
			node->child.push_back(next->child.front());
			node->child.back()->parent = node;
			next->key.erase(next->key.begin());
			next->child.erase(next->child.begin());
			return true;		
		}
		// borrow failed
		return false;	
	}
public:
	BPlusTree(int m){
		this->m = m;
	}
	vector<double> search(int start, int end){
		Node *node = root;
		vector<double> ans;
		int i;
		// go down to the leaf
		while(!node->isLeaf){
			for(i = 0; i < node->key.size(); i++){
				if(start >= node->key[i]) continue;
				break;
			}
			node = node->child[i];
		}
		// search the node
		for(i = 0; i < node->key.size(); i++){
			if(start <= node->key[i] && node->key[i] <= end){
				ans.push_back(node->value[i]);
			}
		}
		// go the next node
		node = node->next;
		while(node && start <= node->key[0] && node->key[0] <= end){
			for(i = 0; i < node->key.size(); i++){
				if(start <= node->key[i] && node->key[i] <= end){
					ans.push_back(node->value[i]);
				}
				else{
					return ans;
				}
			}
			node = node->next;
		}

		return ans;
	}
	bool search(int key, double &value){
		Node *node = root;
		int i;
		// go down to the leaf
		while(!node->isLeaf){
			for(i = 0; i < node->key.size(); i++){
				if(key >= node->key[i]) continue;
				break;
			}
			node = node->child[i];
		}
		for(i = 0; i < node->key.size(); i++){
			if(key == node->key[i]){
				value = node->value[i];
				return true;
			}
		}
		return false;
	}
	void remove(int k){
		if(!root) return;
		Node *node = root;
		int i;
		// go down to the leaf
		while(!node->isLeaf){
			for(i = 0; i < node->key.size(); i++){
				if(k >= node->key[i]) continue;
				break;
			}
			node = node->child[i];
		}
		for(i = 0; i < node->key.size(); i++){
			if(k == node->key[i]){
				// remove from leaf
				node->key.erase(node->key.begin()+i);
				node->value.erase(node->value.begin()+i);
				if(node->key.size() == 0){
					root = _fixDeficientNode(node);
				}
				break;
			}
		}
		return;
	}
	

	void insert(int k, double v){
		if(!root){
			root = new Node();
			root->init(ISLEAF, m);
			root->key.push_back(k);
			root->value.push_back(v);
			return;
		}
		root = _insertToLeaf(root, k, v);
	}
	void dumpLeaf(){
		Node *node = root;
		if(!node){
			cout<<"null\n";
			return;
		}
		while(!node->isLeaf){
			node = node->child[0];
		}
		cout<<"leaf:\n";
		while(node){
			cout<<"[";
			for(int i = 0; i < node->key.size(); i++){
				cout<<"<"<<node->key[i]<<"->"<<node->value[i]<<"> ";
			}
			cout<<"]\n";
			node = node->next;
		}
	}
	void dump(){
		_dump(root,0);
	}
	void _dump(Node *node, int level){
		if(!node){
			cout<<"null\n";
			return;
		}
		for(int i = 0; i < level; i++){
			cout<<" ";
		}
		if(node->isLeaf){
			cout<<"\\leaf[";
			for(int i = 0; i < node->key.size(); i++){
				cout<<"<"<<node->key[i]<<"->"<<node->value[i]<<"> ";
			}
			cout<<"]\n";
		}
		else{
			cout<<"\\inte[";
			for(int i = 0; i < node->key.size(); i++){
				cout<<node->key[i]<<" ";
			}
			cout<<"]\n";
			for(int i = 0; i < node->child.size(); i++){
				_dump(node->child[i], level+4);
			}
		}
	}
};

int main(int argc, char const *argv[]){
	BPlusTree *t;
	ifstream ifs (argv[1]);
	ofstream out ("output_file.txt");
	string line;
	if(ifs.is_open()){
		while(getline(ifs,line)){
			size_t findLP = line.find("(");
			size_t findRP = line.find(")");
			string command = line.substr(0, findLP);
			string param = line.substr(findLP+1, findRP-findLP-1);
			// cout<<"cmd="<<command<<endl;
			// cout<<"param="<<param<<endl;
			if(command.compare(0, 10,"Initialize") == 0){
				int m = stoi(param);
				// cout<<m;
				t = new BPlusTree(m);
			}
			else if(command.compare(0, 6, "Insert") == 0){
				size_t comma = param.find(",");
				string keyStr = param.substr(0, comma);
				string vStr = param.substr(comma+1);
				int key = stoi(keyStr);
				double v = stod(vStr);
				t->insert(key,v);
				// t->dump();
			}
			else if(command.compare(0, 6, "Delete") == 0){
				int key = stoi(param);
				t->remove(key);
				// t->dump();
			}
			else if(command.compare(0, 6, "Search") == 0){
				size_t comma = param.find(",");
				if(comma == string::npos){
					double value;
					int key = stoi(param);
					if(t->search(key, value)){
						out<<value<<endl;
					}
					else{
						out<<"Null\n";
					}
				}
				else{
					string startStr = param.substr(0, comma);
					string endStr = param.substr(comma+1);
					int start = stoi(startStr);
					int end = stoi(endStr);
					vector<double> ans = t->search(start, end);
					for(int i = 0; i < ans.size(); i++){
						if(i != 0) out<<",";
						out<<ans[i];
					}
					out<<endl;
				}
			}
		}
		ifs.close();
		out.close();
	}
	return 0;
}