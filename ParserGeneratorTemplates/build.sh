#!/bin/sh
g++ -lGL -lglut -lboost_regex LexicalAnalyzer.cpp Parser.cpp Main.cpp -o parseTreeBuilder
