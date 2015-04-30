// Copyright (c) 2014 Anonymized. All rights reserved.
//
// Code submitted as supplementary material for manuscript:
// "Dependency Recurrent Neural Language Models for Sentence Completion"
// Do not redistribute.

#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <assert.h>
#include "json.h"
#include "ReadJson.h"
#include "CorpusUnrollsReader.h"

using namespace std;


/**
 * Trim a word
 */
string const ReadJson::Trim(const string &word) const {
  assert(word.length() > 1);
  string res(word);
  if (res[0] == '"') {
    res = res.substr(1, res.length()-1);
  }
  if (res[res.length()-1] == '"') {
    res = res.substr(0, res.length()-1);
  }
  return res;
}


/**
 * Parse a token
 */
size_t const ReadJson::ParseToken(const string &json_element, JsonToken &tok) const {

  //cout << "parseToken: " << json_element << endl;

  size_t len = json_element.length();
  if (len < 14) { return 0; }
  size_t begin = 0;

  // Avoid situations with empty tokens []
  if ((json_element[0] == '[') && (json_element[1] == ']'))
    return 2;

  // Consume the [
  if (json_element[0] == '[') { begin++; }
  // Parse the token number
  size_t end = json_element.find(",", begin);
  assert(end != string::npos);
  string pos_string = json_element.substr(begin, end - begin);
  int token_pos = stoi(pos_string);
  begin = end + 1;
  assert(begin < len);

  // Consume the space and the first "
  if (json_element[begin] == ' ') { begin++; }
  if (json_element[begin] == '"') { begin++; }
  // Parse the word and trim the "
  end = json_element.find("\", ", begin);
  assert(end != string::npos);
  end = end + 1;
  string token_word = json_element.substr(begin, end - begin);
  if (token_word.length() <= 1) {
    cout << json_element << endl;
  }
  assert(token_word.length() > 1);
  token_word = Trim(token_word);
  begin = end + 1;
  assert(begin < len);

  // Parse the discount
  end = json_element.find(",", begin);
  assert(end != string::npos);
  string discount_string = json_element.substr(begin, end - begin);
  double token_discount = stod(discount_string);
  begin = end + 1;
  assert(begin < len);

  // Consume the space and the first "
  if (json_element[begin] == ' ') { begin++; }
  if (json_element[begin] == '"') { begin++; }
  // Parse the label
  end = json_element.find("]", begin);
  assert(end != string::npos);
  string token_label = json_element.substr(begin, end - begin);
  assert(token_label.length() > 2);
  token_label = Trim(token_label);

  // Fill the token
  tok.pos = token_pos;
  tok.word = token_word;
  tok.discount = token_discount;
  tok.label = token_label;

  //cout << "token: " << token_pos << " " << token_word
  //     << " " << token_discount << " " << token_label << endl;
  return end;
}


/**
 * Parse an unroll
 */
size_t const ReadJson::ParseUnroll(const string &json_unrolls,
                                   vector<JsonToken> &unroll) const {

  //cout << "parseUnroll: " << json_unrolls << endl;

  size_t end_unroll = json_unrolls.find("]]", 0);
  assert(end_unroll != string::npos);

  // Avoid situations with empty unrolls []
  if ((json_unrolls[0] == '[') && (json_unrolls[1] == ']'))
    return 2;
  assert(json_unrolls[0] == '[');
  assert(json_unrolls[1] == '[');
  string json_tokens(json_unrolls.substr(0, end_unroll + 1));
  size_t begin = 1;
  size_t end = end_unroll + 1;

  while (begin < end_unroll + 1) {
    // Find the next end of the token
    //cout << "parseToken[" << begin << ", " << end << "]\n" << flush;
    JsonToken tok;
    end = ParseToken(json_tokens.substr(begin, end - begin), tok);
    if (end > 0) {
      // Store the token in the unroll
      unroll.push_back(tok);
      // Go to next token
      // Consume the ], comma and space
      begin += end;
      if (json_tokens[begin] == ']') { begin++; }
      if (json_tokens[begin] == ',') { begin++; }
      if (json_tokens[begin] == ' ') { begin++; }
      end = end_unroll + 1;
    } else
      break;
  }

  return end_unroll;
}


/**
 * Parse a sentence
 */
size_t const ReadJson::ParseSentence(const string &json_sentences,
                                     vector<vector<JsonToken>> &sentence) const {

  //cout << "parseSentence: " << json_sentences << endl;
  assert(json_sentences.length() >= 6);
  size_t end_sentence = json_sentences.find("]]]", 0);
  assert(end_sentence != string::npos);

  // Avoid situations with empty sentences []
  if ((json_sentences[0] == '[') && (json_sentences[1] == ']'))
    return 2;
  assert(json_sentences[0] == '[');
  assert(json_sentences[1] == '[');
  assert(json_sentences[2] == '[');
  string json_unrolls(json_sentences.substr(0, end_sentence + 2));
  size_t begin = 1;
  size_t end = end_sentence + 2;

  while (begin < end_sentence + 2) {
    // Find the next end of the token
    //cout << "parseUnroll[" << begin << ", " << end << "]\n" << flush;
    vector<JsonToken> unroll;
    end = ParseUnroll(json_unrolls.substr(begin, end - begin),
                      unroll);
    if (end > 2) {
      // Store the unroll in the sentence
      sentence.push_back(unroll);
    }
    // Go to the next unroll
    begin += end;
    // Consume the ], comma and space
    if (json_unrolls[begin] == ']') { begin++; }
    if (json_unrolls[begin] == ']') { begin++; }
    if (json_unrolls[begin] == ',') { begin++; }
    if (json_unrolls[begin] == ' ') { begin++; }
    end = end_sentence + 2;
  }

  return end_sentence;
}


/**
 * Parse a book
 */
size_t const ReadJson::ParseBook(const string &json_book,
                                 vector<vector<vector<JsonToken>>> &book) const {

  //cout << "parseBook: " << json_book << endl;
  assert(json_book.length() >= 8);
  size_t end_book = json_book.find("]]]]", 0);
  if (end_book == string::npos) {
    end_book = json_book.find("]]], []]", 0);
    if (end_book == string::npos) {
      end_book = json_book.find("]]], [], []]", 0);
      assert(end_book != string::npos);
    }
  }

  assert(json_book[0] == '[');
  size_t begin = 1;
  if ((json_book[begin] == '[') && (json_book[begin+1] == ']') &&
      (json_book[begin+2] == ',') && (json_book[begin+3] == ' ')) {
    begin += 4;
  }
  if ((json_book[begin] == '[') && (json_book[begin+1] == ']') &&
      (json_book[begin+2] == ',') && (json_book[begin+3] == ' ')) {
    begin += 4;
  }
  assert(json_book[begin] == '[');
  assert(json_book[begin + 1] == '[');
  assert(json_book[begin + 2] == '[');
  string json_sentences(json_book.substr(0, end_book + 3));
  size_t end = end_book + 3;

  while (begin < end_book + 3) {
    // Find the next end of the token
    //cout << "parseSentence[" << begin << ", " << end << "]\n" << flush;
    vector<vector<JsonToken>> sentence;
    end = ParseSentence(json_sentences.substr(begin, end - begin),
                        sentence);
    if (end > 2) {
      // Store the sentence
      book.push_back(sentence);
    }
    // Go to next sentence
    begin += end;
    // Consume the ], ], ], comma and space
    if (json_sentences[begin] == ']') { begin++; }
    if (json_sentences[begin] == ']') { begin++; }
    if (json_sentences[begin] == ']') { begin++; }
    if (json_sentences[begin] == ',') { begin++; }
    if (json_sentences[begin] == ' ') { begin++; }
    end = end_book + 3;
  }

  return end_book;
}

//#define USE_OLD_JSON

/**
 * Constructor: read a text file in JSON format.
 * If required, insert words and labels to the vocabulary.
 * If required, insert tokens into the current book.
 */
ReadJson::ReadJson(const string &filename,
                   CorpusUnrolls &corpus,
                   bool insert_vocab,
                   bool read_book,
                   bool merge_label_with_word) {

#ifdef USE_OLD_JSON
  // Used for parsing the JSON data file
  char *errorPos = 0;
  const char *errorDesc = 0;
  int errorLine = 0;
  block_allocator allocator(10000000); // 1MB blocks
#endif

#ifdef USE_OLD_JSON
  // Read the file with JSON data
  FILE *fin = fopen(filename.c_str(), "rb");
  if (fin == NULL) {
    cerr << "Could not open file " << filename << "...\n";
  }
  char *source = (char *)allocator.malloc(100000000); // 100MB
  int a = 0;
  while (!feof(fin)) {
    char ch = fgetc(fin);
    source[a] = ch;
    a++;
  }
  fclose(fin);
#else
  cout << "Reading book " << filename << "..." << endl;
  ifstream t(filename);
  string book_text((istreambuf_iterator<char>(t)),
                   istreambuf_iterator<char>());

  vector<vector<vector<JsonToken>>> sentences;
  cout << "Parsing book " << filename << "..." << endl;
  ParseBook(book_text, sentences);
  cout << "Parsing done.\n";
#endif

#ifdef USE_OLD_JSON
  // Parse the JSON and keep a pointer to the root of the book
  json_value *_root = json_parse(source,
                                 &errorPos, &errorDesc, &errorLine,
                                 &allocator);
  if (_root == NULL) {
    cerr << "\nError at line " << errorLine << endl;
    cerr << errorDesc << endl << errorPos << endl;
  }
#endif

  // Pointer to the current book
  BookUnrolls *book = &(corpus.m_currentBook);

  // First, iterate over sentences
  int numSentences = 0;

#ifdef USE_OLD_JSON
  for (json_value *s = _root->first_child;
       s;
       s = s->next_sibling) {
#else
  for (int idx_sentence = 0; idx_sentence < sentences.size(); idx_sentence++) {
#endif

    int numUnrollsInThatSentence = 0;
    bool isNewSentence = true;

    // Second, iterate over unrolls in each sentence
#ifdef USE_OLD_JSON
    for (json_value *u = s->first_child;
         u;
         u = u->next_sibling) {
#else
    vector<vector<JsonToken>> unrolls = sentences[idx_sentence];
    for (int idx_unroll = 0; idx_unroll < unrolls.size(); idx_unroll++) {
#endif
      bool isNewUnroll = true;

      // Third, iterate over tokens in each unroll
#ifdef USE_OLD_JSON
      for (json_value *token = u->first_child;
           token;
           token = token->next_sibling) {
#else
      vector<JsonToken> tokens = unrolls[idx_unroll];
      for (int idx_token = 0; idx_token < tokens.size(); idx_token++) {
#endif

        // Process the token to get:
        // its position in sentence,
        // word, discount and label
#ifdef USE_OLD_JSON
        string tokenWordAsContext(""), tokenWordAsTarget(""), tokenLabel("");
        double tokenDiscount = 0;
        int tokenPos = -1;
        ProcessToken(token, tokenPos, tokenWordAsTarget, tokenDiscount, tokenLabel);
        tokenWordAsContext = tokenWordAsTarget;
#else
        string tokenWordAsTarget = tokens[idx_token].word;
        string tokenLabel = tokens[idx_token].label;
        int tokenPos = tokens[idx_token].pos;
        double tokenDiscount = 1.0 / (tokens[idx_token].discount);
        string tokenWordAsContext(tokenWordAsTarget);
#endif

        // Concatenate word with label, when it is used as context?
        if (merge_label_with_word) {
          tokenWordAsContext += ":" + tokenLabel;
        }

        // Shall we insert new words/labels
        // into the vocabulary?
        if (insert_vocab) {
          if (merge_label_with_word) {
            if (tokenLabel == "LEAF") {
              // Insert target word to vocabulary
              corpus.InsertWord(tokenWordAsTarget, tokenDiscount);
            } else {
              // Insert concatenated context word and label to vocabulary
              corpus.InsertWord(tokenWordAsContext, tokenDiscount);
            }
          } else {
            // Insert word and label to two different vocabularies
            corpus.InsertWord(tokenWordAsContext, tokenDiscount);
            if (tokenLabel != "LEAF") {
              corpus.InsertLabel(tokenLabel);
            }
          }
        }
        // Insert new words to the book
        int wordIndexAsContext = 0, wordIndexAsTarget = 0, labelIndex = 0;
        if (merge_label_with_word) {
          wordIndexAsContext = corpus.LookUpWord(tokenWordAsContext);
          wordIndexAsTarget = corpus.LookUpWord(tokenWordAsTarget);
        } else {
          wordIndexAsContext = corpus.LookUpWord(tokenWordAsContext);
          wordIndexAsTarget = wordIndexAsContext;
          labelIndex = corpus.LookUpLabel(tokenLabel);
        }
        book->AddToken(isNewSentence, isNewUnroll,
                       tokenPos, wordIndexAsContext, wordIndexAsTarget,
                       tokenDiscount, labelIndex);
        // We are no longer at beginning of a sentence or unroll
        isNewSentence = false;
        isNewUnroll = false;
      }
#ifndef USE_OLD_JSON
      tokens.clear();
#endif
      numUnrollsInThatSentence++;
    }
#ifndef USE_OLD_JSON
    unrolls.clear();
#endif
    numSentences++;
  }
#ifndef USE_OLD_JSON
  sentences.clear();
  book_text.clear();
#endif
  cout << "ReadJSON: " << filename << endl;
  cout << "          (" << numSentences << " sentences, including empty ones; ";
  cout << book->NumTokens() << " tokens)\n";
  if (insert_vocab) {
    cout << "          Corpus now contains " << corpus.NumWords()
    << " words and " << corpus.NumLabels() << " labels\n";
  }
}


/**
 * Parse a token in the JSON parse tree to fill token data
 */
void ReadJson::ProcessToken(const json_value *_token,
                            int & tokenPos,
                            string & tokenWord,
                            double & tokenDiscount,
                            string & tokenLabel) const {
  // Safety check
  if (_token == NULL) {
    tokenPos = -1;
    tokenWord = "";
    tokenDiscount = 0;
    tokenLabel = "";
    return;
  }
  // Get the position of the token in the unroll
  json_value *element = _token->first_child;
  tokenPos = element->int_value;
  // Get the string of the word in the token in the unroll
  element = element->next_sibling;
  tokenWord = string(element->string_value);
  // Get the discount of the token in the unroll
  element = element->next_sibling;
  tokenDiscount = (double) 1.0 / (element->int_value);
  // Get the label of the token in the unroll
  element = element->next_sibling;
  tokenLabel = string(element->string_value);
}
