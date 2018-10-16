/*
How to run Validation script:
g++ --std=c++11 validationScript.cpp -o validation
./validation ../data/true_gene_fusion_events_expected.txt <output_file_after_fusion_detection_with_gene_pairs> 
*/

#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>

using namespace std;

int main(int argc, char *argv[])
{
    cout<<"Validation started\n\n";
    string ln, referenceName, pos1, pos2, gene_chromosome_name, readSeq;
    string gene_A, gene_B, gene_a;
    unordered_map<string, string> fusion_genes, fusion_genes_found;
    unordered_multimap<string, string> true_positive, false_positive;
    unordered_map<string, string> genomes;
    unordered_map<string, unordered_set<string>> read_gene_map;
    long countFusionEvent = 0;
//    if(argc == 1)
    if(argc < 3)
    {
        cout<<"2 Input files for validation not specified\n"<<endl;
        exit(1);
    }
    ifstream fl(argv[1]);
    while(getline(fl, ln))
    {
        istringstream iss(ln);
        if(iss >> gene_A >> gene_B)
        {
            if(gene_A == "nameA" && gene_B == "nameB")
                continue;
//            cout << gene_A << ", " << gene_B << endl;
            fusion_genes[gene_A] = gene_B;
        }
    }
    cout << "\nTotal number of expected Fusion events = " << fusion_genes.size() << endl;
    ifstream f2(argv[2]);
    while(getline(f2, ln))
    {   
        istringstream iss(ln);
        if(iss >> gene_A >> gene_B)
        {   
            if(gene_A == "nameA" && gene_B == "nameB")
                continue;
//            cout << gene_A << ", " << gene_B << endl;
            fusion_genes_found[gene_A] = gene_B;
        }
    }
    cout << "Total number of Fusion events found = " << fusion_genes_found.size() << endl;
    for(auto it : fusion_genes_found)
    {
        auto i1 = fusion_genes.find(it.first);
        auto i2 = fusion_genes.find(it.second);
        if((i1 != fusion_genes.end() && i1->second == it.second) || (i2 != fusion_genes.end() && i2->second == it.second))
           true_positive.emplace(it.first, it.second);
        else
            false_positive.emplace(it.first, it.second);
    }

    cout << "Total number of True positive Fusion events detected: " << true_positive.size() << endl << "True positive Fusion events detected are: \n";
    for(auto i : true_positive)
        cout << "\t" << i.first << " - " << i.second << endl;
    cout << "\nTotal number of False positive Fusion events detected: " << false_positive.size() << endl;
    return 0;
}

