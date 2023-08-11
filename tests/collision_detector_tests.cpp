#include "test_utils.h"

SCENARIO("No collision detected") {
    WHEN("when no items") {
    	collision_detector::ItemGatherer gath{
            {}, {{{0, 1}, {5, 1}, 3.5}, {{0, 0}, {20, 20}, 3.5}, {{-5, 0}, {15, 3}, 3.5}}};
        THEN("And no events") {
            auto events = collision_detector::FindGatherEvents(gath);
            CHECK(events.empty());
        }
    }
    WHEN("no gatherers") {
    	collision_detector::ItemGatherer gath{
            {{0, {1, 3}, 3.}, {1, {0, 0}, 3.}, {2, {-10, 0}, 3.}}, {}};
        THEN("No events") {
            auto events = collision_detector::FindGatherEvents(gath);
            CHECK(events.empty());
        }
    }
    WHEN("multiple items on a way of gatherer") {
    	collision_detector::ItemGatherer gath{{
            {0, {9, 0.27}, .1}, {1, {8, 0.24}, .1}, {2, {7, 0.21}, .1}, {3, {6, 0.18}, .1},
            {4, {5, 0.15}, .1}, {5, {4, 0.12}, .1}, {6, {3, 0.09}, .1}, {7, {2, 0.06}, .1}, {8, {1, 0.03}, .1}, {9, {0, 0.0}, .1},
            {10, {-1, 0}, .1}, }, 
            { {{0, 0}, {10, 0}, 0.1},
        }};
        THEN("Gathered items in right order") {
            auto events = collision_detector::FindGatherEvents(gath);
            CHECK_THAT(
                events,
                EqualsRange(std::vector{
                    collision_detector::GatheringEvent{9, 0,0.0, 0.0},
                    collision_detector::GatheringEvent{8, 0,0.0009, 0.1},
                    collision_detector::GatheringEvent{7, 0,0.0036, 0.2},
                    collision_detector::GatheringEvent{6, 0,0.0081, 0.3},
                    collision_detector::GatheringEvent{5, 0,0.0144, 0.4},
                    collision_detector::GatheringEvent{4, 0,0.0225, 0.5},
                    collision_detector::GatheringEvent{3, 0,0.0324, 0.6},
                }, EventsComparator()));
        }
    }
    WHEN("there are several gatherers on the way to one item") {
    	collision_detector::ItemGatherer gath{{ {0, {0, 0}, 0.0}, },
                                            { {{-5, 0}, {5, 0}, 1.}, {{0, 1}, {0, -1}, 1.},
                                                {{-11, 11}, {101, -101}, 0.5},
                                                {{-101, 100}, {11, -11}, 0.5},
					     }
        };
        THEN("And faster gatherer get an item") {
            auto events = collision_detector::FindGatherEvents(gath);
            CHECK(events.front().gatherer_id == 2);
        }
    }
    WHEN("Gatherers were static") {
    	collision_detector::ItemGatherer gath{{ {0, {0, 0}, 10.5},},
                                            { {{-3.1, 0}, {-3.1, 0}, 1.5}, {{0.0, 0}, {0.0, 0}, 1.5},
                                                {{-11.5, 10}, {-11.5, 10}, 120} }
        };
        THEN("No events detected") {
            auto events = collision_detector::FindGatherEvents(gath);

            CHECK(events.empty());
        }
    }
}
