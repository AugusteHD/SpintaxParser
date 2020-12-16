#include <iostream>
#include <vector>
#include <random>
#include <list>
#include <string>
#include <fstream>
#include <assert.h>

using namespace std;

class OBJ;

enum {ATOM, SET , SENTENCE};


class Spintax
{
public:
    int type;
public:
    Spintax(int type_) : type(type_) {};
    virtual ~Spintax() = default;
    Spintax(const Spintax &) = delete;
    Spintax(Spintax &&) = default;

    virtual string random() const = 0;
    virtual int count() const = 0;

    virtual void append(Spintax * other) = 0;

    virtual void print() const = 0;
};

class Atom : public Spintax
{
public:
    string phrase;
public:
    Atom(const string & phrase_)
        : Spintax(ATOM),
          phrase(phrase_)
    {}
    ~Atom() = default;

    string random() const override final
    {
        return phrase;
    };
    int count() const override final
    {
        return 1;
    };
    void append(Spintax * other)
    {
        abort();
    };

    void print() const override final
    {
        cout << phrase;
    }
};

class Set : public Spintax
{
public:
    vector<Spintax *> alternatives;
public:
    Set()
        : Spintax(SET)
    {}
    ~Set()
    {
        for (Spintax * s : alternatives)
            delete s;
    }

    string random() const override final
    {
        int random = rand() * alternatives.size() / RAND_MAX ;
        return alternatives[random]->random();
    };
    int count() const override final
    {
        int result = 0;
        for (Spintax * s : alternatives)
            result += s->count();

        return result;
    };

    void append(Spintax * other) override
    {
        alternatives.emplace_back(other);
    }

    void print() const override final
    {
        cout << '{';
        auto iter = alternatives.begin();
        (*iter)->print();
        for (++iter; iter != alternatives.end() ;++iter )
        {
            cout << '|';
            (*iter)->print();
        }
        cout << '}';
    }
};

class Sentence : public Spintax
{
public:
    list<Spintax *> sequence;
public:
    Sentence()
        : Spintax(SENTENCE)
    {}

    Sentence(Sentence &&) = default;

    ~Sentence()
    {
        for (Spintax * s : sequence)
            delete s;
    }

    string random() const override final
    {
        string result="";
        for (Spintax * s : sequence)
            result += s->random();

        return result;
    };
    int count() const override final
    {
        int result = 1;
        for (Spintax * s : sequence)
            result *= s->count();

        return result;
    };

    void append(Spintax * other) override
    {
        sequence.emplace_back(other);
    }

    void print() const override final
    {
        auto iter = sequence.begin();
        (*iter)->print();
        for (++iter; iter != sequence.end() ;++iter )
        {
            (*iter)->print();
        }
    }
};

Sentence parse(std::ifstream & file )
{
char b;
    std::list<Spintax *> pile;
    Sentence X;
    pile.emplace_back(&X);

    while (file.get(b))
    {
        char16_t a;
        if (b>0)
            a = b;
        else
            a = b+256;

        if (a == '{')
        {
            Spintax * last = pile.back();
            if (last->type == SENTENCE)
            {
                Set * newLast = new Set();
                last->append(newLast);
                pile.emplace_back(newLast);
            }
            else if (last->type == SET)
            {
                Sentence * newSentence = new Sentence;
                last->append(newSentence);
                pile.emplace_back(newSentence);

                Set * newLast = new Set();
                newSentence->append(newLast);
                pile.emplace_back(newLast);
            }
            else    // if (last->type == ATOM)
            {
                pile.pop_back();
                if (pile.size()>0)
                {

                    Spintax * secondLast = pile.back();
                    if (secondLast->type == SENTENCE)
                    {
                        Set * newLast = new Set();
                        secondLast->append(newLast);
                        pile.emplace_back(newLast);
                    }
                    else    // if (secondLast->type == SET)
                    {
                        static_cast<Set *>(secondLast)->alternatives.pop_back();

                        Sentence * newSentence = new Sentence;
                        newSentence->append(last);
                        secondLast->append(newSentence);
                        pile.emplace_back(newSentence);

                        Set * newLast = new Set();
                        newSentence->append(newLast);
                        pile.emplace_back(newLast);
                    }
                }
                else
                {
                    Sentence * newSentence = new Sentence;
                    newSentence->append(last);
                    pile.emplace_back(newSentence);

                    Set * newLast = new Set();
                    newSentence->append(newLast);
                    pile.emplace_back(newLast);
                }
            }
        }
        else if (a == '}')
        {
            Spintax * last;
            do
            {
                last = pile.back();
                pile.pop_back();
            }
            while (last->type != SET);
        }
        else if (a == '|')
        {
            Spintax * last = pile.back();
            if ( last->type == SET )
            {
                if ( static_cast<Set * >(last)->alternatives.size() == 0 )
                {
                    Atom * emptyAtom = new Atom("");
                    static_cast<Set * >(last)->alternatives.push_back(emptyAtom);
                }
            }
            else
            {
                while (last->type != SET)
                {
                    pile.pop_back();
                    last = pile.back();
                }
            }
        }
        else if (a == '\n')
        {
            Spintax * last;
            last = pile.back();
            if (last->type == ATOM)
                pile.pop_back();

            break;
        }
        else    // character
        {
            Spintax * last = pile.back();
            if (last->type == ATOM)
            {
                static_cast<Atom * >(last)->phrase.push_back(a);
            }
            else
            {
                Atom * newlast = new Atom(string(1, a));
                last->append(newlast);
                pile.emplace_back(newlast);
            }
        }
    }

    if(pile.size() !=1 )
    {
        cout << "pile.size() = " << pile.size() << endl;
        abort();
    }
    return X;
}


int main()
{
    const std::string file = "phrase.txt";

    std::ifstream Myfile;
    Myfile.open(file, std::ios::in );

    const Sentence X = parse(Myfile);

    cout <<  "Number of combinaisons = " << X.count() << endl;

    for (int i=0; i<10; ++i)
    {
        cout << "random sentence : "<< X.random() << endl;
    }

    cout << endl;
    //X.print();

    return 0;

}
