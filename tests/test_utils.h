#include <cmath>
#include <functional>
#include <sstream>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include "../src/collision_detector.h"

namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& ge) {
        std::ostringstream ostream;
        ostream << "GatheringEvent: [" << ge.gatherer_id << ge.item_id << ge.sq_distance << ge.time << "]";

        return ostream.str();
    }
};
}

namespace {

template <typename Range, typename Predicate>
struct RangeMatcher : Catch::Matchers::MatcherGenericBase {
       RangeMatcher(Range const& range, Predicate predicate)
        : range_{range}
        , predicate_{predicate} {
    }

    template <typename OtherRange>
    bool match(const OtherRange& other) const 
    {
        return std::equal(std::begin(range_), std::end(range_), std::begin(other), std::end(other), predicate_);
    }

    std::string describe() const override {
        return "Equals: " + Catch::rangeToString(range_);
    }

private:
    const Range& range_;
    Predicate predicate_;
};

template <typename Range, typename Predicate>
auto EqualsRange(const Range& range, Predicate prediate) {
    return RangeMatcher<Range, Predicate>{range, prediate};
}

class EventsComparator {
public:
    bool operator()(const collision_detector::GatheringEvent& ev1, const collision_detector::GatheringEvent& ev2) 
    {
        if (ev1.gatherer_id != ev2.gatherer_id || ev1.item_id != ev2.item_id)
            return false;

        const double epsilon = 1e-10;

        if ((std::abs(ev1.sq_distance - ev2.sq_distance) > epsilon) || (std::abs(ev1.time - ev2.time) > epsilon))
            return false;

        return true;
    }
};

}
