#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <hunspell/hunspell.hxx>

using namespace std;

struct BSTNode {
    string word;
    BSTNode* left;
    BSTNode* right;

    BSTNode(const string& w) {
        word = w;
        left = nullptr;
        right = nullptr;
    }
};

class BSTDictionary {
private:
    BSTNode* root;

    BSTNode* insertRec(BSTNode* node, const string& word) {
        if (node == nullptr)
            return new BSTNode(word);
        if (word < node->word)
            node->left = insertRec(node->left, word);
        else if (word > node->word)
            node->right = insertRec(node->right, word);
        return node;
    }

    bool searchRec(BSTNode* node, const string& word) const {
        if (!node)
            return false;
        if (word == node->word)
            return true;
        if (word < node->word)
            return searchRec(node->left, word);
        else
            return searchRec(node->right, word);
    }

    void deleteTree(BSTNode* node) {
        if (!node)
            return;
        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

public:
    BSTDictionary() {
        root = nullptr;
    }

    ~BSTDictionary() {
        deleteTree(root);
    }

    void insert(const string& word) {
        root = insertRec(root, word);
    }

    bool search(const string& word) const {
        return searchRec(root, word);
    }
};

string toLowercase(const string& str) {
    string result = str;
    for (char& ch : result) {
        ch = tolower(ch);
    }
    return result;
}

bool isExceptionWord(const string& word) {
    return word == "is" || word == "his" || word == "this" || word == "was" || word == "has" || word == "as";
}

string removeSuffixes(string word) {
    if (word.length() > 3) {
        if (word.substr(word.length() - 3) == "ies")
            return word.substr(0, word.length() - 3) + "y";
        else if (word.substr(word.length() - 3) == "ing")
            return word.substr(0, word.length() - 3);
        else if (word.substr(word.length() - 2) == "ed" || word.substr(word.length() - 2) == "es" || word.substr(word.length() - 2) == "ly")
            return word.substr(0, word.length() - 2);
    }

    if (word.length() > 1 && word.back() == 's' && !isExceptionWord(word))
        return word.substr(0, word.length() - 1);

    return word;
}

bool loadDictionary(const string& filename, BSTDictionary& bst) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open dictionary file.\n";
        return false;
    }

    string word;
    while (getline(file, word)) {
        if (!word.empty()) {
            bst.insert(toLowercase(word));
        }
    }

    file.close();
    return true;
}

void splitPunctuation(const string& word, string& prefix, string& core, string& suffix) {
    size_t start = 0;
    while (start < word.length() && ispunct(word[start]))
        prefix += word[start++];

    size_t end = word.length();
    while (end > start && ispunct(word[end - 1]))
        suffix = word[end - 1] + suffix, --end;

    core = word.substr(start, end - start);
}


int main() {
    BSTDictionary bst;
    string dictionaryFile = "dict.txt";

    if (!loadDictionary(dictionaryFile, bst))
        return 1;

    Hunspell hunspell("en_US.aff", "en_US.dic");

    cout << "Dictionary loaded.\n";
    cout << "Auto-correct misspelled words? (y/n): ";
    char autoChoice;
    cin >> autoChoice;
    cin.ignore();

    bool autoCorrect = (autoChoice == 'y' || autoChoice == 'Y');

    cout << "Input file name  (e.g., Input.txt): ";
    string inputFileName;
    getline(cin, inputFileName);

    cout << "Output file name (e.g., output.html): ";
    string outputFileName1;
    getline(cin, outputFileName1);

    cout << "Output file name (e.g., output.txt): ";
    string outputFileName2;
    getline(cin, outputFileName2);

    ifstream inputFile(inputFileName);
    ofstream outputFile1(outputFileName1);
    ofstream outputFile2(outputFileName2);


    if (!inputFile.is_open() || !outputFile1.is_open() || !outputFile2.is_open()) {
        cerr << "Error: Cannot open input/output file.\n";
        return 1;
    }

    outputFile1 << "<html><body style='font-family:Arial; line-height:1.6;'>\n";
    outputFile1 << "<h2 style='color:#2c3e50;'>Spell Correction Output</h2>\n";
    string line;
    while (getline(inputFile, line)) {
        istringstream iss(line);
        string word;
        bool first = true;

        while (iss >> word) {
            string prefix = "", core = "", suffix = "";
            splitPunctuation(word, prefix, core, suffix);

            string lowerCore = toLowercase(core);
            string stemmed = removeSuffixes(lowerCore);

            bool found = bst.search(lowerCore) || bst.search(stemmed);
            string corrected = core;

            if (!found) {
                if (hunspell.spell(core)) {
                    corrected = core;
                }
                else {
                    vector<string> suggestions = hunspell.suggest(core);
                    if (!suggestions.empty()) {
                        if (autoCorrect) {
                            corrected = suggestions[0];
                            cout << "Auto-corrected '" << core << "' -> '" << corrected << "'\n";
                        }
                        else {
                            cout << "Misspelled: '" << core << "'\n";
                            cout << "0. Keep original\n";
                            for (size_t i = 0; i < suggestions.size(); ++i)
                                cout << (i + 1) << ". " << suggestions[i] << '\n';

                            int choice;
                            cout << "Choose (0-" << suggestions.size() << "): ";
                            cin >> choice;
                            cin.ignore();

                            if (choice >= 1 && choice <= (int)suggestions.size())
                                corrected = suggestions[choice - 1];
                            else
                                cout << "Keeping original.\n";
                        }
                    }
                    else {
                        corrected = core + "[incorrect]";
                    }
                }
            }

            if (!first)
            {
                outputFile1 << " ";
                outputFile2 << " ";
            }

            if (corrected != core) {

                outputFile1 << prefix
                    << "<span style='color:red'>" << core << "</span> &rarr;"
                    << "<span style='color:green;'>" << corrected << "</span>"
                    << suffix;
                outputFile2 << prefix << corrected << suffix;
            }
            else {
                outputFile1 << prefix << corrected << suffix;
                outputFile2 << prefix << corrected << suffix;
            }

            first = false;
        }

        outputFile1 << "<br>\n";
    }

    outputFile1 << "</body></html>";
    inputFile.close();
    outputFile1.close();
    outputFile2.close();

    std::cout << "Correction done. Output saved to '" << outputFileName1 << "' and '" << outputFileName2;
    return 0;
}
