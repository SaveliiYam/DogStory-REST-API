#include <exception>  

using namespace std;  
class MapNotFoundException : public exception
{  
    public:  
        const char * what() const throw()  
        {  
            return "Map not found\n";  
        }  
};  

class EmptyNameException : public exception
{  
    public:  
        const char * what() const throw()  
        {  
            return "Empty player name specified\n";  
        }  
};  

class ParsingJsonException : public exception
{  
    public:  
        const char * what() const throw()  
        {  
            return "Invalid Json\n";  
        }  
};  

class MetodNotAllowedException : public exception
{  
    public:  
        const char * what() const throw()  
        {  
            return "Invalid HTTP method\n";  
        }  
};  

class PlayerAbsentException : public exception
{
    public:
        const char * what() const throw()
        {
            return "No player with specified token\n";
        }
};


class DogSpeedException : public exception
{
    public:
        const char * what() const throw()
        {
            return "Impossible to set dog speed\n";
        }
};

class InvalidSessionException : public exception
{
    public:
        const char * what() const throw()
        {
            return "Impossible to find session with specified auth info\n";
        }
};

class BadDeltaTimeException : public exception
{
    public:
        const char * what() const throw()
        {
            return "Wrong value for timeDelta parameter\n";
        }
};
