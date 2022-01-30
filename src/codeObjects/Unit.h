#ifndef TKOMSIUNITS_CODE_OBJECTS_UNIT_H_INCLUDED
#define TKOMSIUNITS_CODE_OBJECTS_UNIT_H_INCLUDED

#include "error/ErrorHandler.h"
#include "lexer/Lexer.h"
#include "utils/printUtils.h"
#include <map>
#include <sstream>
#include <ostream>

namespace codeobj {
class Unit;
}

std::ostream& operator<<(std::ostream &os, const codeobj::Unit &unit);

namespace codeobj {

class Unit {
public:
    Unit() : isScalar_(true) {}

    explicit Unit(::Unit unitToken) : isScalar_(false) {
        numerator_.insert({ unitToken.unit, unitToken });
    }

    void combineWithUnit(Token op, const Unit &unit) {
        if (std::get<std::string>(op.value) == "*") {
            for (auto &&num : unit.numerator_) {
                if (auto it = numerator_.find(num.first); it != numerator_.end()) {
                    if (it->second.prefix != num.second.prefix) {
                        ErrorHandler::handleFromParser("Cannot combine units with different prefixes");
                    }
                    it->second.power += num.second.power;
                } else {
                    numerator_.insert(num);
                }
            }
            for (auto &&den : unit.denominator_) {
                if (auto it = denominator_.find(den.first); it != denominator_.end()) {
                    if (it->second.prefix != den.second.prefix) {
                        ErrorHandler::handleFromParser("Cannot combine units with different prefixes");
                    }
                    it->second.power += den.second.power;
                } else {
                    denominator_.insert(den);
                }
            }
            reduceFraction();
        } else if (std::get<std::string>(op.value) == "/") {
            for (auto &&num : unit.numerator_) {
                if (auto it = denominator_.find(num.first); it != denominator_.end()) {
                    if (it->second.prefix != num.second.prefix) {
                        ErrorHandler::handleFromParser("Cannot combine units with different prefixes");
                    }
                    it->second.power += num.second.power;
                } else {
                    denominator_.insert(num);
                }
            }
            for (auto &&den : unit.denominator_) {
                if (auto it = numerator_.find(den.first); it != numerator_.end()) {
                    if (it->second.prefix != den.second.prefix) {
                        ErrorHandler::handleFromParser("Cannot combine units with different prefixes");
                    }
                    it->second.power += den.second.power;
                } else {
                    numerator_.insert(den);
                }
            }
            reduceFraction();
        } else {
            ErrorHandler::handleFromParser("Multiplicative operator different than '*' and '/'");
        }
        
    }

    void multWithUnit(const Unit &unit) {
        combineWithUnit(Token{TokenType::OP_MULT, "*"}, unit);
    }
    void divWithUnit(const Unit &unit) {
        combineWithUnit(Token{TokenType::OP_MULT, "/"}, unit);
    }
    
    bool isScalar() const noexcept {
        return isScalar_;
    }
    
    std::string toString() const {
        std::ostringstream os;
        os << *this;
        return os.str();
    }
    
    bool isAddCompatibileWith(const Unit &other) const {
        auto areUnitsCompatibile = [](const std::pair<UnitType, ::Unit> &left, const std::pair<UnitType, ::Unit> &right) {
                return std::make_pair(left.second.unit, left.second.power) == std::make_pair(right.second.unit, right.second.power);
            };
        return
            std::equal(numerator_.cbegin(), numerator_.cend(),
                other.numerator_.cbegin(), other.numerator_.cend(),
                areUnitsCompatibile
            )
            &&
            std::equal(denominator_.cbegin(), denominator_.cend(),
                other.denominator_.cbegin(), other.denominator_.cend(),
                areUnitsCompatibile
            );
    }

private:
    void updateIsScalar() {
        if (numerator_.empty() && denominator_.empty()) {
            isScalar_ = true;
        } else {
            isScalar_ = false;
        }
    }

    void reduceFraction() {
        // reduce same units in numerator and denominator
        for (auto numIt = numerator_.begin(); numIt != numerator_.end();) {
            if (auto denIt = denominator_.find(numIt->first); denIt != denominator_.end()) {
                ::Unit &top = numIt->second;
                ::Unit &bottom = denIt->second;
                if (top.prefix != bottom.prefix) {
                    ErrorHandler::handleFromParser("Cannot combine units with different prefixes");
                }
                top.power -= bottom.power;
                if (top.power < 0) {
                    bottom.power = -top.power;
                    numIt = numerator_.erase(numIt);
                } else {
                    denominator_.erase(denIt);
                    if (top.power == 0) {
                        numIt = numerator_.erase(numIt);
                    } else {
                        ++numIt;
                    }
                }
            } else {
                ++numIt;
            }
        }
        updateIsScalar();
    }

private:
    // map, not unordered_map, beceuse the elements have to be ordered
    std::map<UnitType, ::Unit> numerator_;
    std::map<UnitType, ::Unit> denominator_;
    bool isScalar_;
    
    friend std::ostream& ::operator<<(std::ostream &os, const Unit &unit);
};

} // namespace codeobj

inline std::ostream& operator<<(std::ostream &os, const codeobj::Unit &unit) {
    if (unit.isScalar()) {
        return os << "[1]";
    }
    os << "[(";
    bool first = true;
    for (auto &&[t, u] : unit.numerator_) {
        (void)t;
        if (!first) {
            os << '*';
        }
        os << u;
        first = false;
    }
    os << ")/(";
    first = true;
    for (auto &&[t, u] : unit.denominator_) {
        (void)t;
        if (!first) {
            os << '*';
        }
        os << u;
        first = false;
    }
    os << ")]";
    return os;
}

#endif // TKOMSIUNITS_CODE_OBJECTS_UNIT_H_INCLUDED