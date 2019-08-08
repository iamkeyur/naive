#include <iostream>
#include "../naive.h"

int main() {
    Index<int> i(2);
    std::vector<std::string> doc1, doc2;
    doc1.push_back("a b c");
    doc1.push_back("hello world I'm Keyur Patel. She is Payal Patel");
    doc2.push_back("c d e");
    doc2.push_back("lorem hell ipsum Patel");

    i.addDocumentToIndex(1, doc1);
    i.addDocumentToIndex(2, doc2);

    std::vector<double> fieldsBoost;
    fieldsBoost.push_back(1);
    fieldsBoost.push_back(1);

    QueryResult<int> q;
    std::set<int> removed;
    std::vector<QueryResult<int>> r = q.query(i, fieldsBoost, 1.2, 0.75, removed, "payal Patel");
    for (auto i : r) {
        std::cout << "Key : " << i.key << "   Score : " << i.score << std::endl;
    }

    return 0;
}
