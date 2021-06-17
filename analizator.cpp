#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <fstream>
#include <cstring>
#include <algorithm>
using namespace std;

class Token {
public:
    enum TYPE { CUVANTCHEIE, IDENTIFICATOR, OPERATOR, DELIMITATOR, CONSTANTANUMERICA, CONSTANTAREALA, CONSTANTASTRING, CONSTANTACARACTER, COMENTARIU, UNDEFINED };

    Token(){}
    Token( TYPE vType, int vValue ) {
        this->mTokenType = vType;
        this->mValue = vValue;
    }

    //Setters
    void SetTokenType( TYPE newTokenType ) {this->mTokenType = newTokenType; }
    void SetValue( int newValue ) { this->mValue = newValue; }
    
    //Getters
    int GetValue() { return this->mValue; }
    Token::TYPE GetTokenType() { return this->mTokenType; }

    static std::string GetString( TYPE t ) { 
        
        switch (t)
        {
        case CUVANTCHEIE :
            return "Cuvant cheie";
        case IDENTIFICATOR :
            return "Identificator";
        case OPERATOR :
            return "Operator";
        case DELIMITATOR :
            return "Delimitator";
        case CONSTANTANUMERICA :
            return "Constanta numerica";
        case CONSTANTAREALA :
            return "Constanta reala";
        case CONSTANTASTRING :
            return "Constanta string";
        case CONSTANTACARACTER :
            return "Constanta caracter";
        case COMENTARIU :
            return "Comentariu";
        default:
            return "Eroare";
        };

    }

private:
    TYPE mTokenType;
    int mValue;
};

class State {
public:
    const static int ERR_TRANZITION = -1;

    //Setters
    void SetTokenType( Token::TYPE newType ) { mTokenType =  newType; }
    void SetIsFinale( bool newFinal ) { mIsFinale = newFinal; }
    void SetId( int newId ) { mId = newId; }
    
    //Getters
    int GetId() { return mId; }
    Token::TYPE GetTokenType() { return mTokenType; }
    std::vector< std::pair<int,char> >& GetTranzitions() { return mTransitions; }
    
    
    bool IsFinal() { return mIsFinale == true; }
    
    int ChangeState( char charRead ) {
        for ( unsigned int i = 0; i < mTransitions.size(); ++i ) {
            if ( mTransitions[ i ].second == charRead ) {
                return mTransitions[ i ].first;
            }
        }
        return ERR_TRANZITION;
    }

    void AddTranzition( int newState, char trans ) {
        mTransitions.push_back( std::make_pair( newState, trans ) );
    }


private:
    int mId;
    bool mIsFinale;
    Token::TYPE mTokenType;
    std::vector< std::pair<int,char> > mTransitions;
};

class Analizator{
public:
    
    Analizator( string definitionPath, string keyWordsPath, string sourcePath ) {
        readDefinition(definitionPath);
        readKeywords(keyWordsPath);
        loadSource(sourcePath);
        startingCondition();
        mLastIdentificator = false;
    }

    Token GetToken() {
        char currChar = GetInputChar();

        if ( mCurrState.GetId() == mAutomata[ 0 ].GetId()  ) {
            if ( strchr( "\r\n\t  ", currChar ) != NULL ) {
                return GetToken();
            }
        }
 
        int newStateId = mCurrState.ChangeState( currChar );
        if ( newStateId == State::ERR_TRANZITION || mReadBuffer.empty() ) {
            if ( mCurrState.IsFinal() ) {
                bool isKeyword = std::find( mKeyWords.begin(), mKeyWords.end(), mWorkingBuffer ) != mKeyWords.end();

                if( std::find( mTokenValues.begin(),mTokenValues.end(), mWorkingBuffer ) == mTokenValues.end() ) {
                    mTokenValues.push_back(mWorkingBuffer);
                } 

                int tokenIndex = 0;
                for ( unsigned int i = 1; i <= mTokenValues.size(); ++i ) {
                    if ( mTokenValues[ i ] == mWorkingBuffer ) {
                        tokenIndex = i;
                    }
                }

                mReadBuffer.push_front(currChar);
                Token toRet = Token( isKeyword ? Token::CUVANTCHEIE : mCurrState.GetTokenType(),tokenIndex);

                if ( (mLastIdentificator == true) && (mCurrState.GetTokenType() == Token::CONSTANTAREALA) ) {
                    
                    mLastIdentificator = false;
                    if ( mWorkingBuffer != "." ) {
                        startingCondition();
                        return Token( Token::CONSTANTAREALA, tokenIndex );
                    }
                    startingCondition();
                    return Token( Token::OPERATOR, tokenIndex );
                }


                startingCondition();
                mLastIdentificator = mCurrState.GetTokenType() == Token::IDENTIFICATOR;

                if ( toRet.GetTokenType() == Token::COMENTARIU ) {
                    return GetToken();
                }

                return toRet;
            }  else {
                return Token(Token::UNDEFINED, -1 );
            }
        } else {
            mWorkingBuffer += currChar;
            mCurrState = mAutomata[newStateId-1];
            return GetToken();
        }
    }

    std::pair<std::string,std::string> GetReadableToken( Token toConvert ) {
        return std::make_pair( Token::GetString( toConvert.GetTokenType() ), mTokenValues[ toConvert.GetValue() ] );
    }

    int FindStateById( int id ) {
        for ( unsigned int i = 0; i < mAutomata.size(); ++i ) {
            if ( mAutomata[ i ].GetId() == id ) {
                return i;
            }
        }
        return -1;
    }


private:
    char GetInputChar() {
        if ( mReadBuffer.empty() ) {
            throw 404;
        }
        
        char toReturn = mReadBuffer.front();
        mReadBuffer.pop_front();
        return toReturn;
    }

    
    void readKeywords(std::string path) {
        std::fstream readStream;
        readStream.open( path.c_str(), ios::in );
        
        std::string key;
        while( readStream >> key ) {
            mKeyWords.push_back(key);
        }

        readStream.close();
    }

    void readDefinition( std::string definitionPath) {
        
        std::fstream readStream;
        readStream.open( definitionPath.c_str(), ios::in );

        int currState, nextState, isFinal, typeToken;
        int N, M;
        std::string tran;
        
        readStream >> N;
        for( int i = 1; i <= N; ++i )
        {   
            readStream >> typeToken >> isFinal;
            State newState;
            newState.SetId( i );
            newState.SetIsFinale( isFinal );
            newState.SetTokenType( static_cast<Token::TYPE>(typeToken) );
            mAutomata.push_back(newState);
        }
        
        readStream >> M;
        for ( int i = 1; i <= M; ++i ) {
            readStream >> currState >> nextState >> tran;
            if ( tran == "NEWLINE " ) {
                tran = "\n";
            }   
            for ( unsigned int j = 0; j < tran.size(); ++j ) {
                mAutomata[ currState-1 ].AddTranzition( nextState, tran[ j ] );
            }
        }

        readStream >> M;
        for ( int i = 1; i <= M; ++i ) {
            readStream >> currState >> nextState >> tran;
            if ( tran == "NEWLINE" ) {
                tran = "\n";
            }   
            
            for ( int j = 0; j < 128; ++j ) {
                if( tran.find((char)(j)) == std::string::npos ) {
                    mAutomata[ currState-1 ].AddTranzition( nextState, (char)j );
                }
            }
        }

        for( int i = mAutomata[ 12 ].GetTranzitions().size() - 1; i >= 0 ; --i )
        {
            if(mAutomata[ 12 ].GetTranzitions()[i].second == '\n' ) {
                    mAutomata[ 12 ].GetTranzitions().erase( mAutomata[ 12 ].GetTranzitions().begin() + i );
            }
        }

        readStream.close();
    }

    void loadSource( std::string filePath ) {
        std::fstream readStream;
        readStream.open( filePath.c_str(),ios::in);
        char read;

        while ( !readStream.eof() ) {
            readStream.get(read);
            mReadBuffer.push_back(read);
        }

        readStream.close();
    }

    void startingCondition() {
        mWorkingBuffer = "";
        mCurrState = mAutomata[ 0 ];
    }

    std::vector<std::string> mTokenValues;
    std::vector<State> mAutomata;
    std::vector<std::string> mKeyWords;

    std::deque<char> mReadBuffer;
    std::string mWorkingBuffer;

    bool mLastIdentificator;
    
    State mCurrState;
};


int main( int argc, char* argv[] )
{
    Analizator anal = Analizator("negative.txt", "keywords.txt","test.txt");

    int NMAX = 1000;
    try {
        while ( NMAX > 0 ) {
            NMAX--;
            Token newToken = anal.GetToken();
            std::pair<std::string,std::string> rez = anal.GetReadableToken(newToken);
            std::cout << rez.first << " " << rez.second << "\n";
            if ( rez.second == "" || rez.first == "Eroare" ) {
                return 0;
            } 
        }
    } catch( int e ) {
        std::cout << "Am ajuns la final !";
    }
    
    return 0;
}