//
// Created by Charlie Huguenard on 12/28/2018.
//

#include "catch.h"
#include "../lib/Graph.h"

TEST_CASE("dummy test", "[asdfasdfasd]")
{
    REQUIRE(true);
    dc::Graph g;
    REQUIRE(g.getNumAudioInputs() == 0);
}
