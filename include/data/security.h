#pragma once

#include <iostream>
#include <string>

using namespace std;


/**
 * Abstract class for securities.
 */
class Security {
    public:
        /**
         * Constructor to create security.
         */
        Security(const string& base_, const string& quote_):
                base(base_), quote(quote_) {}

        /**
         * Destructor.
         */
        virtual ~Security() {}

        /**
         * Getter for base
         */
        string getBase() const {return base;}

        /**
         * Getter for quote
         */
        string getQuote() const {return quote;}

        /**
         * Override << operator
         */
        friend std::ostream& operator<<(std::ostream& os, const Security& sec) {
            os << sec.base << "/" << sec.quote;
            return os;
        }

        /**
         * Equal operator. 
         * Compares if the base and quote are the same.
         */
        friend bool operator==(const Security& lhs, const Security& rhs) {
            return (lhs.base == rhs.base) && (lhs.quote == rhs.quote);
        }

        /**
         * Not equal operator.
         */
        friend bool operator!=(const Security& lhs, const Security& rhs) {
            return !(lhs == rhs);
        }

        /**
         * Hash used for unordered map
         */
        struct Hash {
            size_t operator()(const Security& sec) const {
                // Combine the hash values of base and quote
                size_t baseHash = hash<string>{}(sec.base);
                size_t quoteHash = hash<string>{}(sec.quote);
                return baseHash ^ (quoteHash << 1); // XOR and shift
            }
        };

    protected:
        const string base;  /*< Name of the base security (one before the slash) */
        const string quote; /*< Name of the quote security (one after the slash) */
};