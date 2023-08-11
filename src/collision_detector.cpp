#include "collision_detector.h"
#include <cassert>
namespace collision_detector
{

    CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c)
    {
        // Проверим, что перемещение ненулевое.
        // Тут приходится использовать строгое равенство, а не приближённое,
        // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
        // расстояние.
        assert(b.x != a.x || b.y != a.y);
        const double u_x = c.x - a.x;
        const double u_y = c.y - a.y;
        const double v_x = b.x - a.x;
        const double v_y = b.y - a.y;
        const double u_dot_v = u_x * v_x + u_y * v_y;
        const double u_len2 = u_x * u_x + u_y * u_y;
        const double v_len2 = v_x * v_x + v_y * v_y;
        const double proj_ratio = u_dot_v / v_len2;
        const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

        return CollectionResult(sq_distance, proj_ratio);
    }

    std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider &provider)
    {
        std::vector<GatheringEvent> events;

        const double epsilon = 1e-10;
        for (size_t i = 0; i < provider.GatherersCount(); ++i)
        {
            Gatherer gath = provider.GetGatherer(i);

            if ((std::abs(gath.start_pos.x - gath.end_pos.x) <= epsilon) &&
                (std::abs(gath.start_pos.y - gath.end_pos.y) <= epsilon))
            {
                continue;
            }
            for (size_t j = 0; j < provider.ItemsCount(); ++j)
            {
                Item item = provider.GetItem(j);
                auto result = TryCollectPoint(gath.start_pos, gath.end_pos, item.position);

                if (result.IsCollected(gath.width + item.width))
                {
                    GatheringEvent evt(j, i, result.sq_distance, result.proj_ratio);
                    events.push_back(evt);
                }
            }
        }

        std::sort(events.begin(), events.end(), [](const GatheringEvent &evt1, const GatheringEvent &evt2)
                  { return evt1.time < evt2.time; });

        return events;
    }
} // namespace collision_detector