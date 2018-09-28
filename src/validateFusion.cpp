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
    cout<<"Started\n\n";
    string ln, referenceName, pos1, pos2, gene_chromosome_name, readSeq;
    unordered_map<string, string> genomes;
    unordered_map<string, unordered_set<string>> read_gene_map;
    long countFusionEvent = 0;
    if(argc == 1)
    {
        cout<<"No input file specified\n"<<endl;
        exit(1);
    }
    ifstream fl(argv[1]);
    while(getline(fl, ln))
    {
        istringstream iss(ln);
        if (!(iss >> referenceName >> gene_chromosome_name >> pos1 >> pos2 >> readSeq))
            break;
        if((gene_chromosome_name[0] == 'E') && (gene_chromosome_name[1] == 'N'))
        {
            string gene_for_the_current_read;
            stringstream iss3(gene_chromosome_name);
            if(getline(iss3, gene_for_the_current_read, '|'))
                if(getline(iss3, gene_for_the_current_read, '|'))
                    if(getline(iss3, gene_for_the_current_read, '|'))
                        if(getline(iss3, gene_for_the_current_read, '|'))
                            if(getline(iss3, gene_for_the_current_read, '|'))
                                if(getline(iss3, gene_for_the_current_read, '|'))
                                {
                                    auto itr_read = read_gene_map.find(referenceName);
                                    if(itr_read == read_gene_map.end())
                                    {
                                        unordered_set<string> gene_for_this_read;
                                        gene_for_this_read.emplace(gene_for_the_current_read);
                                        read_gene_map.emplace(referenceName, gene_for_this_read);
                                    }
                                    else
                                        itr_read->second.emplace(gene_for_the_current_read);
                                }
        }
    }
    cout << "*** Fusion event found ***\n";
    for(const auto &genItr : read_gene_map)
    {
        if(genItr.second.size() > 1)
        {
/*
            vector<string> vec_gene;
            for(auto itr_mp = genItr.second.begin(); itr_mp != genItr.second.end(); itr_mp++)
                vec_gene.push_back(*itr_mp);
            // Assuming fusion by the reads mapping to 2 different genes
            auto it_g1 = genomes.find(vec_gene[0]);
            auto it_g2 = genomes.find(vec_gene[1]);
            if(it_g1 != genomes.end())
              if(it_g1->second == vec_gene[1])
                continue;
            if(it_g2 != genomes.end())
              if(it_g2->second == vec_gene[0])
                continue;
            genomes.emplace(vec_gene[0], vec_gene[1]);
            for(auto fused_genes : vec_gene)
                cout << fused_genes << ",\t";
            cout << endl;
*/
	    countFusionEvent++;
        }
    }
    cout << "\nTotal number of Gene Fusion event detected: " << countFusionEvent << endl;
}
