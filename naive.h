#include <bits/stdc++.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

template <class T>
class DocumentDetails {
   public:
    T key;
    std::vector<int> fieldLengths;

    DocumentDetails(T k, std::vector<int> v) : key{k}, fieldLengths{v} {}
};

template <class T>
class DocumentPointer {
   public:
    DocumentPointer<T> *next;
    DocumentDetails<T> *details;
    std::vector<int> termFrequency;
};

template <class T>
class InvertedIndexNode {
   public:
    int charCode;
    InvertedIndexNode<T> *next;
    InvertedIndexNode<T> *firstChild;
    DocumentPointer<T> *firstDoc;

    InvertedIndexNode(int number) {
        charCode = number;
        next = nullptr;
        firstChild = nullptr;
        firstDoc = nullptr;
    }
    InvertedIndexNode *findInvertedIndexNode(std::string term) {
        InvertedIndexNode<T> *node = this;
        for (std::string::iterator it = term.begin(); node != nullptr && it != term.end(); ++it) {
            node = node->findInvertedIndexChildNodeByCharCode((int)*it);
        }
        return node;
    }

    InvertedIndexNode *findInvertedIndexChildNodeByCharCode(int charCode) {
        InvertedIndexNode *child = this->firstChild;
        while (child != nullptr) {
            if (child->charCode == charCode) {
                return child;
            }
            child = child->next;
        }
        return nullptr;
    }

    void addInvertedIndexChildNode(InvertedIndexNode<T> *child) {
        if (this->firstChild != nullptr) {
            child->next = this->firstChild;
        }
        this->firstChild = child;
    }

    InvertedIndexNode *createInvertedIndexNodes(std::string term, int start) {
        InvertedIndexNode<T> *node = this;
        std::string::iterator it = term.begin();
        std::advance(it, start);
        for (; it != term.end(); ++it) {
            InvertedIndexNode<T> *newNode = new InvertedIndexNode<T>((int)*it);
            node->addInvertedIndexChildNode(newNode);
            node = newNode;
        }
        return node;
    }
    void addInvertedIndexDoc(DocumentPointer<T> *doc) {
        if (this->firstDoc != nullptr) {
            doc->next = this->firstDoc;
        }
        this->firstDoc = doc;
    }
};

template <class T>
class Index {
   public:
    std::map<T, DocumentDetails<T> *> docs;
    std::vector<std::pair<double, double>> fields;
    InvertedIndexNode<T> *root = new InvertedIndexNode<T>(0);

    Index(int fieldsNum) {
        for (int i = 0; i < fieldsNum; i++) {
            fields.push_back(std::make_pair(0.0, 0.0));
        }
        root = new InvertedIndexNode<T>(0);
    }

    template <class D>
    void addDocumentToIndex(T key, std::vector<D> dd) {
        std::vector<int> fieldLengths;
        std::map<std::string, std::vector<int>> termCounts;
        for (int i = 0; i < fields.size(); i++) {
            D filedValue = dd[i];

            if (filedValue == "") {
                fieldLengths.push_back(0);
            } else {
                int filteredTermsCount = 0;
                double sum, avg;
                std::tie(sum, avg) = fields[i];
                std::istringstream iss(filedValue);
                std::vector<D> terms{std::istream_iterator<D>{iss}, std::istream_iterator<D>{}};
                for (int j = 0; j < terms.size(); j++) {
                    std::string term = terms[j];
                    if (term != "") {
                        filteredTermsCount++;
                        if (!(termCounts.count(term) > 0)) {
                            std::vector<int> cnt(fields.size());
                            termCounts.emplace(term, cnt);
                        }
                        termCounts[term][i] += 1;
                    }
                }
                sum += filteredTermsCount;
                avg = sum / (docs.size() + 1);
                fieldLengths.push_back(filteredTermsCount);
                fields[i] = std::make_pair(sum, avg);
            }
        }

        DocumentDetails<T> *details = new DocumentDetails<T>(key, fieldLengths);
        docs.insert({key, details});

        std::map<std::string, std::vector<int>>::iterator it = termCounts.begin();
        while (it != termCounts.end()) {
            InvertedIndexNode<T> *node = root;

            for (std::string::size_type i = 0; i < it->first.size(); i++) {
                if (node->firstChild == nullptr) {
                    node = node->createInvertedIndexNodes(it->first, i);
                    break;
                }
                InvertedIndexNode<T> *nextNode = node->findInvertedIndexChildNodeByCharCode((int)it->first[i]);
                if (nextNode == nullptr) {
                    node = node->createInvertedIndexNodes(it->first, i);
                    break;
                }
                node = nextNode;
            }
            DocumentPointer<T> *dp = new DocumentPointer<T>();
            dp->details = details;
            dp->next = nullptr;
            dp->termFrequency = it->second;
            node->addInvertedIndexDoc(dp);
            it++;
        }
    }
};

template <class T>
class QueryResult {
   public:
    T key;
    double score;
    QueryResult() {}

    QueryResult(T k, double v) : key{k}, score{v} {}

    std::vector<QueryResult<T>> query(Index<T> index, std::vector<double> fieldsBoost, double bm25k1, double bm25b, std::set<T> removed, std::string s) {
        typedef std::function<bool(std::pair<T, double>, std::pair<T, double>)> Comparator;
        // Defining a lambda function to compare two pairs. It will compare two pairs using second field
        Comparator compFunctor =
            [](std::pair<T, double> elem1, std::pair<T, double> elem2) {
                return elem1.second > elem2.second;
            };
        std::map<T, double> scores;
        std::istringstream iss(s);
        std::vector<std::string> terms{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

        for (int i = 0; i < terms.size(); i++) {
            std::string term = terms[i];
            if (term != "") {
                std::vector<std::string> expandedTerms = expandTerm(index, term);
                std::set<T> visitedDocuments;
                for (int j = 0; j < expandedTerms.size(); j++) {
                    std::string eTerm = expandedTerms[j];
                    int help = 1 + (1 / (1 + eTerm.size() - term.size()));
                    double expansionBoost = (eTerm == term ? 1 : log(1.0 + (1.0 / (1.0 + eTerm.size() - term.size()))));
                    InvertedIndexNode<T> *termNode = index.root->findInvertedIndexNode(eTerm);

                    if (termNode != nullptr && termNode->firstDoc != nullptr) {
                        int documentFrequency = 0;
                        DocumentPointer<T> *pointer = termNode->firstDoc;
                        DocumentPointer<T> *prevPointer;

                        while (pointer != nullptr) {
                            if (removed.size() != 0 && (removed.find(pointer->details->key) != removed.end())) {
                                if (prevPointer == nullptr) {
                                    termNode->firstDoc = pointer->next;
                                } else {
                                    prevPointer->next = pointer->next;
                                }
                            } else {
                                prevPointer = pointer;
                                documentFrequency++;
                            }
                            pointer = pointer->next;
                        }

                        if (documentFrequency > 0) {
                            double idf = log(1.0 + (index.docs.size() - documentFrequency + 0.5) / (documentFrequency + 0.5));
                            pointer = termNode->firstDoc;
                            while (pointer != nullptr) {
                                if (removed.size() == 0 || !(removed.find(pointer->details->key) != removed.end())) {
                                    double score = 0;
                                    for (int x = 0; x < pointer->details->fieldLengths.size(); x++) {
                                        double tf = pointer->termFrequency[x];
                                        if (tf > 0) {
                                            // calculating BM25 tf
                                            int fieldLength = pointer->details->fieldLengths[x];
                                            double sum, avg;
                                            std::tie(sum, avg) = index.fields[x];
                                            double avgFieldLength = avg;
                                            tf = ((bm25k1 + 1.0) * tf) / (bm25k1 * ((1.0 - bm25b) + bm25b * (fieldLength / avgFieldLength)) + tf);
                                            score += tf * idf * fieldsBoost[x] * expansionBoost;
                                        }
                                    }
                                    if (score > 0) {
                                        T key = pointer->details->key;
                                        typename std::map<T, double>::iterator prevScore = scores.find(key);
                                        if (prevScore != scores.end() && (visitedDocuments.find(key) != visitedDocuments.end())) {
                                            scores[key] = (prevScore->second > score ? prevScore->second : score);
                                        } else {
                                            if (prevScore != scores.end()) {
                                                scores[key] = prevScore->second + score;
                                            } else {
                                                scores.insert({key, score});
                                            }
                                        }
                                        visitedDocuments.insert(key);
                                    }
                                }
                                pointer = pointer->next;
                            }
                        }
                    }
                }
            }
        }

        std::vector<QueryResult<T>> result;
        std::set<std::pair<T, double>, Comparator> sortedResult(
            scores.begin(), scores.end(), compFunctor);

        for (std::pair<T, double> element : sortedResult) {
            QueryResult<T> r(element.first, element.second);
            result.push_back(r);
        }

        return result;
    }

    std::vector<std::string>
    expandTerm(Index<T> index, std::string term) {
        InvertedIndexNode<T> *node = index.root->findInvertedIndexNode(term);
        std::vector<std::string> results;
        if (node != nullptr) {
            _expandTerm(node, results, term);
        }
        return results;
    }

    void _expandTerm(InvertedIndexNode<T> *node, std::vector<std::string> &results, std::string term) {
        if (node->firstDoc != nullptr) {
            results.push_back(term);
        }
        InvertedIndexNode<T> *child = node->firstChild;
        while (child != nullptr) {
            _expandTerm(child, results, term + (char)child->charCode);
            child = child->next;
        }
    }
};