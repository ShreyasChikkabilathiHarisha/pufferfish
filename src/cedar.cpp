#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib> // std::abs
#include <cmath> // std::abs
#include <unordered_set>
#include "clipp.h"
#include "cedar.hpp"
#include "EquivalenceClassBuilder.hpp"

#define LEFT true
#define RIGHT true
#define SCALE_FACTOR 1000000

struct CedarOpts {
    std::string taxonomyTree_filename;
    std::string refId2TaxId_filename;
    std::string mapperOutput_filename;
    std::string output_filename;
    std::string level = "species";
    double filterThreshold = 0;
    double eps = 0.001;
    size_t maxIter = 100;
};

Cedar::Cedar(std::string& taxonomyTree_filename, 
                 std::string& refId2TaxId_filename, 
                 std::string pruneLevelIn,
                 double filteringThresholdIn) {

    std::cerr << "KrakMap: Construct ..\n";
    // map rank string values to enum values
    filteringThreshold = filteringThresholdIn;
    pruningLevel = TaxaNode::str2rank(pruneLevelIn);
    std::ifstream tfile;
    uint32_t id, pid;
    std::string rank, name;
    // load the reference id (name) to its corresponding taxonomy id
    tfile.open(refId2TaxId_filename);
    while(!tfile.eof()) {
        tfile >> name >> id;
        refId2taxId[name] = id;
    }
    tfile.close();

    // load the taxonomy child-parent tree and the rank of each node
    tfile.open(taxonomyTree_filename);
    std::string tmp;
    while (!tfile.eof()) {
        tfile >> id >> tmp >> pid >> tmp >> rank >> tmp;
        size_t i = 0;
        while (i < tmp.size()-1 && isspace(tmp[i]))
            i++;
        if (tmp[i] != '|') {
            rank += " " + tmp;
        }
        taxaNodeMap[id] = TaxaNode(id, pid, TaxaNode::str2rank(rank));
        if (taxaNodeMap[id].isRoot()) {
            rootId = id;
            std::cerr << "Root Id : " << id << "\n";
        }
        std::getline(tfile, tmp);
        
    }

    tfile.close(); 
}

bool Cedar::readHeader(std::ifstream& mfile) {
    std::string tmp, readType;
    mfile >> tmp >> readType;
    if (tmp != "#")
        return false;
    if (readType == "LT:S") 
        isPaired = false;
    else if (readType == "LT:P")
        isPaired = true;
    else
        return false;
    std::getline(mfile, tmp);
    return true;
}

void Cedar::loadMappingInfo(std::string mapperOutput_filename) {
    int32_t rangeFactorization{4};
    std::string rid, tname, tmp;// read id, taxa name, temp
    uint64_t lcnt, rcnt, tid, puff_tid, tlen, ibeg, ilen;
    std::cerr << "Cedar: Load Mapping File ..\n";
    std::cerr << "\tMapping Output File: " << mapperOutput_filename << "\n";
    std::ifstream mfile(mapperOutput_filename);
    uint64_t rlen, mcnt; // taxa id, read mapping count, # of interals, interval start, interval length
    uint64_t totalReadCnt = 0, totalUnmappedReads = 0, seqNotFound = 0;
    if (!readHeader(mfile)) {
        std::cerr << "ERROR: Invalid header for mapping output file.\n";
        std::exit(1);
    }
    std::cout<< "is dataset paired end? " << isPaired << "\n";
    while (mfile >> rid >> mcnt) { 
        totalReadCnt++;
        if (totalReadCnt % 100000 == 0) {
            std::cerr << "Processed " << totalReadCnt << " reads\n";
        }
        activeTaxa.clear();
        float readMappingsScoreSum = 0;
        std::vector<std::pair<uint64_t, float>> readPerStrainProbInst;
        readPerStrainProbInst.reserve(10);
        //std::cout << "r" << rid << " " << mcnt << "\n";
        if (isPaired) {
          uint64_t rllen, rrlen;
          mfile >> rllen >> rrlen;
          rlen = rllen + rrlen;
        } else {
          mfile >> rlen;
        }

        if (mcnt != 0) {
            std::set<uint64_t> seen;
            for (size_t mappingCntr = 0; mappingCntr < mcnt; mappingCntr++) {
                mfile >> puff_tid >> tname >> tlen; //txp_id, txp_name, txp_len
                // first condition: Ignore those references that we don't have a taxaId for
                // secon condition: Ignore repeated exactly identical mappings (FIXME thing)
                if (refId2taxId.find(tname) != refId2taxId.end() &&
                    activeTaxa.find(refId2taxId[tname]) == activeTaxa.end()) { 

                    tid = refId2taxId[tname];
                    seqToTaxMap[puff_tid] = tid;
                    activeTaxa.insert(tid);
                    
                    // fetch the taxon from the map
                    TaxaNode taxaPtr;
                    mfile >> lcnt;
                    if (isPaired)
                        mfile >> rcnt;
                    for (size_t i = 0; i < lcnt; ++i) {
                        mfile >> ibeg >> ilen;
                        taxaPtr.addInterval(ibeg, ilen, LEFT);
                    }
                    if (isPaired)
                        for (size_t i = 0; i < rcnt; ++i) {
                            mfile >> ibeg >> ilen;
                            taxaPtr.addInterval(ibeg, ilen, RIGHT);
                        }
                    taxaPtr.cleanIntervals(LEFT);
                    taxaPtr.cleanIntervals(RIGHT);
                    taxaPtr.updateScore();
                    readPerStrainProbInst.emplace_back(puff_tid, static_cast<float>(taxaPtr.getScore())/static_cast<float>(tlen));
                    readMappingsScoreSum += readPerStrainProbInst.back().second;
                }
                else { // otherwise we have to read till the end of the line and throw it away
                    std::getline(mfile, tmp);
                }
            } 
            if (activeTaxa.size() == 0) {
                seqNotFound++;
            } else {
                readCnt++;
                // it->first : strain id
                // it->second : prob of current read comming from this strain id
                for (auto it = readPerStrainProbInst.begin(); it != readPerStrainProbInst.end(); it++) {
                    it->second = it->second/readMappingsScoreSum; // normalize the probabilities for each read
                    // strain[it->first].first : read count for strainCnt
                    // strain[it->first].second : strain length
                    if (strain.find(it->first) == strain.end()) {
                        strain[it->first] = 1.0/static_cast<float>(readPerStrainProbInst.size());
                    }
                    else {
                        strain[it->first] += 1.0/static_cast<float>(readPerStrainProbInst.size());
                    }
                }
                readPerStrainProb.push_back(readPerStrainProbInst);
                // construct the range factorized eq class here 
                std::vector<uint32_t> genomeIDs; genomeIDs.reserve(2*readPerStrainProbInst.size());
                std::vector<double> probs; probs.reserve(readPerStrainProb.size());
                for (auto it = readPerStrainProbInst.begin(); it != readPerStrainProbInst.end(); it++) {
                    genomeIDs.push_back(it->first);
                    probs.push_back(it->second);
                } 
                if (rangeFactorization > 0) {
                    int genomeSize = genomeIDs.size();
                    int rangeCount = std::sqrt(genomeSize) + rangeFactorization;
                    for (int i = 0; i < genomeSize; i++) {
                        int rangeNumber = probs[i] * rangeCount;
                        genomeIDs.push_back(rangeNumber);
                    }
                }
                // TODO: add class here 
                TargetGroup tg(genomeIDs);
                eqb.addGroup(std::move(tg), probs);
            }
        } else {
            totalUnmappedReads++;
            std::getline(mfile, tmp);
        }
    }  
}

bool Cedar::basicEM(size_t maxIter, double eps) {
    eqb.finish();
    auto& eqvec = eqb.eqVec();
    int64_t maxSeqID{-1};
    for (auto& kv : strain) { 
        maxSeqID = (static_cast<int64_t>(kv.first) > maxSeqID) ? static_cast<int64_t>(kv.first) : maxSeqID;
    }

    std::vector<double> newStrainCnt(maxSeqID+1,0.0); 
    std::vector<double> strainCnt(maxSeqID+1);
    for (auto& kv : strain) { 
        strainCnt[kv.first] = kv.second;
    }
    std::cerr << "maxSeqID : " << maxSeqID << "\n";
    std::cerr << "found : " << eqvec.size() << " equivalence classes\n";
    size_t cntr = 0;
    bool converged = false;
    while (cntr++ < maxIter && !converged) {
        // NOTE: I think we should avoid creating a new hash map in each EM iteration.
        // I think the way to do this is to do the EM in terms of the seqIDs, and then only convert to 
        // taxIDs at the end.
        //spp::sparse_hash_map<uint64_t, float> newStrainCnt;
        //newStrainCnt.reserve(eqvec.size());

        // M step
        // Find the best (most likely) count assignment
        for (auto& eqc : eqvec) {
            auto& tg = eqc.first;
            auto& v = eqc.second;
            auto csize = v.weights.size();
            std::vector<double> tmpReadProb(csize);
            double denom{0.0};
            for (size_t readMappingCntr = 0; readMappingCntr < csize; ++readMappingCntr) {
                auto tgt = tg.tgts[readMappingCntr];
               tmpReadProb[readMappingCntr] = v.weights[readMappingCntr] * strainCnt[tgt];
               denom += tmpReadProb[readMappingCntr];
            }
            for (size_t readMappingCntr = 0; readMappingCntr < csize; ++readMappingCntr) {
                auto tgt = tg.tgts[readMappingCntr];
                newStrainCnt[tgt] += v.count * (tmpReadProb[readMappingCntr] / denom);
            }
        }
        /*
        // M step
        // Find the best (most likely) count assignment
        for (auto readIt = readPerStrainProb.begin(); readIt != readPerStrainProb.end(); readIt++) {
            float denum = 0;
            std::vector<float> tmpReadProb(readIt->size());
            size_t readMappingCntr = 0;
            for (auto strainIt = readIt->begin(); strainIt != readIt->end(); strainIt++, readMappingCntr++) {
                tmpReadProb[readMappingCntr] = strainIt->second*strain[strainIt->first]/readCnt;
                denum += tmpReadProb[readMappingCntr];
            }
            readMappingCntr = 0;
            for (auto strainIt = readIt->begin(); strainIt != readIt->end(); strainIt++, readMappingCntr++) {
                if (newStrainCnt.find(strainIt->first) == newStrainCnt.end())
                    newStrainCnt[strainIt->first] = tmpReadProb[readMappingCntr]/denum;
                else
                    newStrainCnt[strainIt->first] += tmpReadProb[readMappingCntr]/denum;
                //std::cout << strain[strainIt->first] << "-" 
                 //         << strainIt->second << "-" << readCnt << "-"
                   //       << strain[strainIt->first]*strainIt->second/readCnt << " ";
            }
        }
        */

        // E step
        // normalize strain probabilities using the denum : p(s) = (count(s)/total_read_cnt) 
        float readCntValidator = 0;
        converged = true;   
        double maxDiff={0.0};
        for (size_t i = 0; i < strainCnt.size(); ++i) {
            readCntValidator += newStrainCnt[i];
            auto adiff = std::abs(newStrainCnt[i] - strainCnt[i]);
            if ( adiff > eps) {
                converged = false;
            }
            maxDiff = (adiff > maxDiff) ? adiff : maxDiff;
            strainCnt[i] = newStrainCnt[i];
            newStrainCnt[i] = 0.0;
        }
        /*
        float readCntValidator = 0;
        converged = true;   
        for (auto it = strain.begin(); it != strain.end(); it++) {
            readCntValidator += it->second;
            if (std::abs(newStrainCnt[it->first] - it->second) > eps) {
                converged = false;
            }
            it->second = newStrainCnt[it->first];
        }
        */
        if (std::abs(readCntValidator - readCnt) > 10) {
            std::cerr << "ERROR: Total read count changed during the EM process\n";
            std::cerr << "original: " << readCnt << " current: " << readCntValidator << " diff: " 
                      << std::abs(readCntValidator - readCnt) << "\n";
            std::exit(1);
        }
        if (cntr > 0 and cntr % 100 == 0) {
            std::cerr << "max diff : " << maxDiff << "\n";
        }
    }
    std::cout << "iterator cnt: " << cntr << "\n";
    decltype(strain) outputMap;
    outputMap.reserve(strain.size());

    for (auto& kv : strain) { 
        outputMap[seqToTaxMap[kv.first]] = strainCnt[kv.first];
    }
    std::swap(strain, outputMap);

    return cntr < maxIter;
}

void Cedar::serialize(std::string& output_filename) {
    std::cerr << "Write results in the file:\n" << output_filename << "\n";
    std::ofstream ofile(output_filename);
    ofile << "taxaId\ttaxaRank\tcount\n";
    for (auto& kv : strain) {
        ofile << kv.first << "\t" << TaxaNode::rank2str(taxaNodeMap[kv.first].getRank()) << "\t" << kv.second << "\n";
    }
    ofile.close();
}

/**
 * "How to run" example:
 * make Pufferfish!
 * In the Pufferfish build directory run the following command:
 * /usr/bin/time src/krakmap 
 * -t /mnt/scratch2/avi/meta-map/kraken/KrakenDB/taxonomy/nodes.dmp  
 * -s /mnt/scratch2/avi/meta-map/kraken/KrakenDB/seqid2taxid.map 
 * -m /mnt/scratch2/avi/meta-map/kraken/puff/dmps/HC1.dmp 
 * -o HC1.out 
 * -l genus (optional)
 * -f 0.8 (optional)
 **/
int main(int argc, char* argv[]) {
  (void)argc;
  using namespace clipp;
  CedarOpts kopts;
  bool showHelp{false};

  auto checkLevel = [](const char* lin) -> void {
    std::string l(lin);
    std::unordered_set<std::string> valid{"species", "genus", "family", "order", "class", "phylum"};
    if (valid.find(l) == valid.end()) {
      std::string s = "The level " + l + " is not valid.";
      throw std::range_error(s);
    }
  };

  auto cli = (
              required("--taxtree", "-t") & value("taxtree", kopts.taxonomyTree_filename) % "path to the taxonomy tree file",
              required("--seq2taxa", "-s") & value("seq2taxa", kopts.refId2TaxId_filename) % "path to the refId 2 taxId file ",
              required("--mapperout", "-m") & value("mapout", kopts.mapperOutput_filename) % "path to the pufferfish mapper output file",
              required("--output", "-o") & value("output", kopts.output_filename) % "path to the output file to write results",
              option("--maxIter", "-i") & value("iter", kopts.maxIter) % "maximum number of EM iteratons (default : 100)",
              option("--eps", "-e") & value("eps", kopts.eps) % "epsilon for EM convergence (default : 0.001)",
              option("--level", "-l") & value("level", kopts.level).call(checkLevel) % "choose between (species, genus, family, order, class, phylum). (default : species)",
              option("--filter", "-f") & value("filter", kopts.filterThreshold) % "choose the threshold [0,1] below which to filter out mappings (default : no filter)",
              option("--help", "-h").set(showHelp, true) % "show help",
              option("-v", "--version").call([]{std::cout << "version 0.1.0\n\n";}).doc("show version")
              );
  decltype(parse(argc, argv, cli)) res;
  try {
    res = parse(argc, argv, cli);
    if (showHelp){
      std::cout << make_man_page(cli, "cedar"); 
      return 0;
    }
  } catch (std::exception& e) {
    std::cout << "\n\nparsing command line failed with exception: " << e.what() << "\n";
    std::cout << "\n\n";
    std::cout << make_man_page(cli, "cedar");
    return 1;
  }

  if(res) {
    Cedar cedar(kopts.taxonomyTree_filename, kopts.refId2TaxId_filename, kopts.level, kopts.filterThreshold);
    cedar.loadMappingInfo(kopts.mapperOutput_filename);
    cedar.basicEM(kopts.maxIter, kopts.eps);
    cedar.serialize(kopts.output_filename);
    return 0;
  } else {
    std::cout << usage_lines(cli, "cedar") << '\n';
    return 1;
  }
  /*
  CLI::App app{"krakMap : Taxonomy identification based on the output of Pufferfish mapper through the same process as Kraken."};
  app.add_option("-t,--taxtree", kopts.taxonomyTree_filename,
                 "path to the taxonomy tree file")
      ->required();
  app.add_option("-s,--seq2taxa", kopts.refId2TaxId_filename, "path to the refId 2 taxaId file")
      ->required();
  app.add_option("-m,--mapperout", kopts.mapperOutput_filename, "path to the pufferfish mapper output file")
      ->required();
  app.add_option("-o,--output", kopts.output_filename, "path to the output file to write results")
      ->required();
  app.add_option("-i,--maxIter", kopts.maxIter, "maximum allowed number of iterations for EM")
      ->required(false);
  app.add_option("-e,--eps", kopts.eps, "epsilon for EM convergence condition")
      ->required(false);
  app.add_option("-l,--level", kopts.level, "choose between (species, genus, family, order, class, phylum). Default:species")
      ->required(false);
  app.add_option("-f,--filter", kopts.filterThreshold, "choose the threshold (0-1) to filter out mappings with a score below that. Default: no filter")
      ->required(false);
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }
  */
}
