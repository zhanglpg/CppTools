#include <string>
#include <sstream>
#include <functional>
#include "commonlibs/algorithms.hpp"
#include <gtest/gtest.h>

using namespace commonlibs;

static G_vvii make_graph(int n) { return G_vvii(n); }

static void add_edge(G_vvii &g, int u, int v, int w)
{
    g[u].push_back(gedge(w, v));
}

// Redirect cout, run f(), restore, return captured text.
static std::string capture_cout(std::function<void()> f)
{
    std::streambuf *old = std::cout.rdbuf();
    std::ostringstream oss;
    std::cout.rdbuf(oss.rdbuf());
    f();
    std::cout.rdbuf(old);
    return oss.str();
}

// Extract the integer that follows "distance  " in print_path output.
// Returns -999 on parse failure.
static int extract_distance(const std::string &output)
{
    const std::string marker = "distance  ";
    auto pos = output.find(marker);
    if (pos == std::string::npos) return -999;
    return std::stoi(output.substr(pos + marker.size()));
}

// ---- Dijkstra return value -------------------------------------------------

TEST(Dijkstra, ReturnsZero)
{
    G_vvii g = make_graph(2);
    add_edge(g, 0, 1, 1);
    shortest_path_algo algo;
    EXPECT_EQ(0, algo.Dijkstra(g, 0));
}

// ---- print_path return values ----------------------------------------------

TEST(Dijkstra, PrintPath_ValidVertex_ReturnsZero)
{
    G_vvii g = make_graph(3);
    add_edge(g, 0, 1, 1);
    add_edge(g, 1, 2, 1);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    std::string out = capture_cout([&]{ EXPECT_EQ(0, algo.print_path(2)); });
    EXPECT_FALSE(out.empty());
}

TEST(Dijkstra, PrintPath_NegativeVertex_ReturnsMinusOne)
{
    G_vvii g = make_graph(2);
    add_edge(g, 0, 1, 5);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);
    EXPECT_EQ(-1, algo.print_path(-1));
}

TEST(Dijkstra, PrintPath_OutOfRangeVertex_ReturnsMinusOne)
{
    G_vvii g = make_graph(2);
    add_edge(g, 0, 1, 5);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);
    EXPECT_EQ(-1, algo.print_path(100));
}

// ---- correctness via print_path output ------------------------------------

TEST(Dijkstra, SingleVertex_DistanceToSelfIsZero)
{
    G_vvii g = make_graph(1);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);
    std::string out = capture_cout([&]{ algo.print_path(0); });
    EXPECT_EQ(0, extract_distance(out));
}

TEST(Dijkstra, TwoVertices_DirectEdge)
{
    G_vvii g = make_graph(2);
    add_edge(g, 0, 1, 7);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto dist = [&](int v) {
        return extract_distance(capture_cout([&]{ algo.print_path(v); }));
    };
    EXPECT_EQ(0, dist(0));
    EXPECT_EQ(7, dist(1));
}

TEST(Dijkstra, LinearChain)
{
    // 0 -1-> 1 -2-> 2 -3-> 3
    G_vvii g = make_graph(4);
    add_edge(g, 0, 1, 1);
    add_edge(g, 1, 2, 2);
    add_edge(g, 2, 3, 3);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto dist = [&](int v) {
        return extract_distance(capture_cout([&]{ algo.print_path(v); }));
    };
    EXPECT_EQ(0, dist(0));
    EXPECT_EQ(1, dist(1));
    EXPECT_EQ(3, dist(2));
    EXPECT_EQ(6, dist(3));
}

TEST(Dijkstra, ShortestVsLongerDirectPath)
{
    // 0->1->2 costs 2; 0->2 direct costs 10
    G_vvii g = make_graph(3);
    add_edge(g, 0, 1, 1);
    add_edge(g, 1, 2, 1);
    add_edge(g, 0, 2, 10);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    std::string out = capture_cout([&]{ algo.print_path(2); });
    EXPECT_EQ(2, extract_distance(out));
}

TEST(Dijkstra, UnreachableVertex_DistanceIsSentinel)
{
    G_vvii g = make_graph(3);
    add_edge(g, 0, 1, 5);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    // Vertex 2 is unreachable → distance is the sentinel 987654321
    std::string out = capture_cout([&]{ algo.print_path(2); });
    EXPECT_EQ(987654321, extract_distance(out));
}

TEST(Dijkstra, MultiplePathsChoosesMinimum)
{
    G_vvii g = make_graph(4);
    add_edge(g, 0, 1, 1);
    add_edge(g, 0, 2, 4);
    add_edge(g, 1, 3, 2);
    add_edge(g, 3, 2, 1);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto dist = [&](int v) {
        return extract_distance(capture_cout([&]{ algo.print_path(v); }));
    };
    EXPECT_EQ(0, dist(0));
    EXPECT_EQ(1, dist(1));
    EXPECT_EQ(4, dist(2));
    EXPECT_EQ(3, dist(3));
}

TEST(Dijkstra, StarGraph)
{
    G_vvii g = make_graph(5);
    add_edge(g, 0, 1, 10);
    add_edge(g, 0, 2, 3);
    add_edge(g, 0, 3, 7);
    add_edge(g, 0, 4, 1);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto dist = [&](int v) {
        return extract_distance(capture_cout([&]{ algo.print_path(v); }));
    };
    EXPECT_EQ(0,  dist(0));
    EXPECT_EQ(10, dist(1));
    EXPECT_EQ(3,  dist(2));
    EXPECT_EQ(7,  dist(3));
    EXPECT_EQ(1,  dist(4));
}

TEST(Dijkstra, EarlyTermination_DestinationReached)
{
    G_vvii g = make_graph(3);
    add_edge(g, 0, 1, 2);
    add_edge(g, 1, 2, 2);
    shortest_path_algo algo;
    EXPECT_EQ(0, algo.Dijkstra(g, 0, 1));
    std::string out = capture_cout([&]{ algo.print_path(1); });
    EXPECT_EQ(2, extract_distance(out));
}

TEST(Dijkstra, ZeroWeightEdge)
{
    G_vvii g = make_graph(3);
    add_edge(g, 0, 1, 0);
    add_edge(g, 1, 2, 5);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto dist = [&](int v) {
        return extract_distance(capture_cout([&]{ algo.print_path(v); }));
    };
    EXPECT_EQ(0, dist(1));
    EXPECT_EQ(5, dist(2));
}

// ---- path reconstruction ---------------------------------------------------

// Parse the path vertices from print_path output.
// Output format:  "shortest path from ...\n3 <== 2 <== 1 <== 0\n"
// Returns the sequence of vertex IDs from destination to source.
static std::vector<int> extract_path(const std::string &output)
{
    std::vector<int> path;
    auto nl = output.find('\n');
    if (nl == std::string::npos) return path;
    std::string pathline = output.substr(nl + 1);

    // Replace "<==" with spaces so we can parse ints directly
    std::string::size_type pos;
    while ((pos = pathline.find("<==")) != std::string::npos)
        pathline.replace(pos, 3, " ");

    std::istringstream iss(pathline);
    int v;
    while (iss >> v)
        path.push_back(v);
    return path;
}

TEST(Dijkstra, PathReconstruction_LinearChain)
{
    G_vvii g = make_graph(4);
    add_edge(g, 0, 1, 1);
    add_edge(g, 1, 2, 2);
    add_edge(g, 2, 3, 3);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto path = extract_path(capture_cout([&]{ algo.print_path(3); }));
    // 3 <== 2 <== 1 <== 0
    ASSERT_EQ(4u, path.size());
    EXPECT_EQ(3, path[0]);
    EXPECT_EQ(2, path[1]);
    EXPECT_EQ(1, path[2]);
    EXPECT_EQ(0, path[3]);
}

TEST(Dijkstra, PathReconstruction_TakesShortcut)
{
    // 0->1->2 cost 2; 0->2 direct cost 10
    G_vvii g = make_graph(3);
    add_edge(g, 0, 1, 1);
    add_edge(g, 1, 2, 1);
    add_edge(g, 0, 2, 10);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto path = extract_path(capture_cout([&]{ algo.print_path(2); }));
    ASSERT_EQ(3u, path.size());
    EXPECT_EQ(2, path[0]);
    EXPECT_EQ(1, path[1]);
    EXPECT_EQ(0, path[2]);
}

TEST(Dijkstra, PathReconstruction_SourceToSelf)
{
    G_vvii g = make_graph(2);
    add_edge(g, 0, 1, 1);
    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto path = extract_path(capture_cout([&]{ algo.print_path(0); }));
    // Source has no predecessor — path is just [0]
    ASSERT_EQ(1u, path.size());
    EXPECT_EQ(0, path[0]);
}

TEST(Dijkstra, LargeGraph_10Vertices)
{
    G_vvii g = make_graph(10);
    // Linear chain 0->1->...->9, weight 1 each
    for (int i = 0; i < 9; ++i)
        add_edge(g, i, i + 1, 1);
    // Shortcut 0->5, weight 3
    add_edge(g, 0, 5, 3);

    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto dist = [&](int v) {
        return extract_distance(capture_cout([&]{ algo.print_path(v); }));
    };
    // Via shortcut 0->5 costs 3 (vs chain cost 5)
    EXPECT_EQ(3, dist(5));
    // To 9: shortcut + chain 5->6->7->8->9 = 3+4 = 7 (vs full chain = 9)
    EXPECT_EQ(7, dist(9));
}

TEST(Dijkstra, DiamondGraph)
{
    //   1
    //  / \
    // 0   3
    //  \ /
    //   2
    G_vvii g = make_graph(4);
    add_edge(g, 0, 1, 1);
    add_edge(g, 0, 2, 5);
    add_edge(g, 1, 3, 2);
    add_edge(g, 2, 3, 1);

    shortest_path_algo algo;
    algo.Dijkstra(g, 0);

    auto dist = [&](int v) {
        return extract_distance(capture_cout([&]{ algo.print_path(v); }));
    };
    // 0->1->3 = 3; 0->2->3 = 6
    EXPECT_EQ(3, dist(3));
}
