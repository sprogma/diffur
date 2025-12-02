#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "astworker.h"
#include "math.h"
#include "string.h"
#include "ctype.h"
#include "stdio.h"
#include "omp.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
    
#include <vector>
#include <map>
#include <unordered_map>

#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace __gnu_pbds;


template <typename TKey, typename TValue>
using ordered_map = tree<TKey, TValue, std::less<TKey>, rb_tree_tag, tree_order_statistics_node_update>;
template <typename TKey, typename TValue>
using ordered_multimap = tree<TKey, TValue, std::less_equal<TKey>, rb_tree_tag, tree_order_statistics_node_update>;
template <typename TKey, typename TValue>
using ordered_multimap_desc = tree<TKey, TValue, std::greater_equal<TKey>, rb_tree_tag, tree_order_statistics_node_update>;


std::map<std::vector<int64_t>, int64_t> hash_map;
std::map<std::vector<int64_t>, int64_t> unordered_hash_map;
std::unordered_map<int64_t, struct node_t *>used;
std::unordered_map<struct node_t *, struct node_t *> tree_parent;
std::unordered_map<struct node_t *, int64_t> tree_depth;
ordered_multimap<double, struct node_t *> tree_map;
ordered_multimap_desc<double, struct node_t *> hotspot_map;


int64_t hash_leaf(struct node_t *v, std::map<std::vector<int64_t>, int64_t> &map)
{
    std::vector<int64_t> childs;
    if (v->type == NODE_TYPE_FLOAT || v->type == NODE_TYPE_IDENTIFER)
    {
        if (v->end - v->start < 0)
        {
            printf("???\n");
            *(int *)NULL = 5;
        }
        childs.resize(v->end - v->start + 1);
        for (int i = 0; i < v->end - v->start; ++i)
        {
            childs[i] = v->start[i];
        }
        childs[v->end - v->start] = v->type;
    }
    else
    {
        childs.resize(1);
        childs[0] = v->type;
    }
    
    if (map.find(childs) != map.end())
    {
        return map[childs];
    }
    else
    {
        int64_t res = map.size() + 1;
        map[childs] = res;
        return res;
    }
}


int64_t tree_hash(struct node_t *v)
{
    if (v == NULL)
    {
        return 0;
    }
    if (!v->hash)
    {
        std::vector<int64_t> childs(v->childs_length + 2);

        for (size_t i = 0; i < v->childs_length; ++i)
        {
            childs[i] = tree_hash(v->childs[i]);
        }

        childs[v->childs_length] = hash_leaf(v, hash_map);
        childs[v->childs_length + 1] = 0;
        if (hash_map.find(childs) != hash_map.end())
        {
            v->hash = hash_map[childs];
        }
        else
        {
            int64_t res = hash_map.size() + 1;
            v->hash = hash_map[childs] = res;
        }
    }
    return v->hash;
}


int64_t tree_unordered_hash(struct node_t *v)
{
    if (!v->unordered_hash)
    {
        std::vector<int64_t> childs(v->childs_length + 1);

        for (size_t i = 0; i < v->childs_length; ++i)
        {
            childs[i] = tree_hash(v->childs[i]);
        }

        childs[v->childs_length] = hash_leaf(v, unordered_hash_map);

        if (v->type == NODE_TYPE_MULDIV || v->type == NODE_TYPE_ADDSUB)
        {
            if (v->childs[1]->type == NODE_TYPE_OP_MUL || v->childs[1]->type == NODE_TYPE_OP_ADD)
            {
                if (childs[2] > childs[0])
                {
                    std::swap(childs[2], childs[0]);
                }
            }
        }

        if (unordered_hash_map.find(childs) != unordered_hash_map.end())
        {
            v->unordered_hash = unordered_hash_map[childs];
        }
        else
        {
            int64_t res = unordered_hash_map.size() + 1;
            v->hash = unordered_hash_map[childs] = res;
        }
    }
    return v->unordered_hash;
}



int is_same(struct node_t *a, struct node_t *b)
{
    if (a->type != b->type)
    {
        return 0;
    }
    if (a->type == NODE_TYPE_FLOAT)
    {
        char *e;
        double x = strtod(a->start, &e);
        double y = strtod(b->start, &e);
        return fabs(x - y) < 1e-6;
    }
    if (a->type == NODE_TYPE_IDENTIFER)
    {
        if (a->end - a->start != b->end - b->start)
        {
            return 0;
        }
        return strncmp(a->start, b->start, a->end - a->start) == 0;
    }
    if (a->childs_length != b->childs_length)
    {
        return 0;
    }
    if (a->type == NODE_TYPE_ADDSUB && 
        a->childs[1]->type == NODE_TYPE_OP_ADD && 
        b->childs[1]->type == NODE_TYPE_OP_ADD)
    {
        return (is_same(a->childs[0], b->childs[0]) && is_same(a->childs[2], b->childs[2])) ||
               (is_same(a->childs[0], b->childs[2]) && is_same(a->childs[2], b->childs[0]));
    }
    if (a->type == NODE_TYPE_MULDIV && 
        a->childs[1]->type == NODE_TYPE_OP_MUL && 
        b->childs[1]->type == NODE_TYPE_OP_MUL)
    {
        return (is_same(a->childs[0], b->childs[0]) && is_same(a->childs[2], b->childs[2])) ||
               (is_same(a->childs[0], b->childs[2]) && is_same(a->childs[2], b->childs[0]));
    }
    for (size_t i = 0; i < a->childs_length; ++i)
    {
        if (!is_same(a->childs[i], b->childs[i]))
        {
            return 0;
        }
    }
    return 1;
}



double cost(struct node_t *node)
{
    if (node->cost < 0.0)
    {
        double res = 1.0;
        for (size_t i = 0; i < node->childs_length; ++i)
        {
            int sign = 0;
            if (node->type == NODE_TYPE_ADDSUB ||
                node->type == NODE_TYPE_MULDIV ||
                node->type == NODE_TYPE_POW)
            {    
                 sign = (node->childs[1]->type == NODE_TYPE_OP_MUL) ||
                       (node->childs[1]->type == NODE_TYPE_OP_ADD);
            }
            double ch_res = cost(node->childs[i]);
            if (node->type == NODE_TYPE_ADDSUB ||
                node->type == NODE_TYPE_MULDIV ||
                node->type == NODE_TYPE_POW)
            {
                if (priority[node->childs[i]->type] + (sign || i == 0) <= priority[node->type])
                {
                    ch_res *= 1.05;
                    ch_res += 2.0;
                }
            }
            res += ch_res;
        }
        if (node->type == NODE_TYPE_POW)
        {
            res *= 1.1;
        }
        if (node->type == NODE_TYPE_MULDIV)
        {
            res *= 1.1;
        }
        if (node->type == NODE_TYPE_MULDIV && node->childs[1]->type == NODE_TYPE_OP_DIV)
        {
            res += 4.0;
        }
        if (node->type == NODE_TYPE_ADDSUB && node->childs[1]->type == NODE_TYPE_OP_SUB)
        {
            res += 1.0;
        }

        node->cost = res;
    }
    return node->cost;
}



struct node_t *mutate_tree_inner(struct node_t *node, double time, int too_big)
{
    struct node_t *curr_node = soft_copy(node);
    /* 1. optimize subnodes */
    // if (node->childs_length > 0)
    // {
    //     size_t opt = rand() % node->childs_length;
    //     curr_node->childs[opt] = mutate_tree_inner(node->childs[opt], time, too_big);
    // }
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        struct node_t *x = mutate_tree_inner(node->childs[i], time, too_big);
        curr_node->childs[i] = x;
    }

    /* 2. optimize this node */
    for (size_t i = 0; i < optimization_rules_len; ++i)
    {        
        if (rand() % 1000 < 200)
        {
            struct node_t *result = optimization_rules[i](curr_node, time);
            
            /* measure profit */
            double was = cost(curr_node);
            double now = cost(result);
            double p = 0.8 * exp(time * (was - now) / 4.0);
            if (time > 0.9)
            {
                p = time * (now - 1e-4 <= was);
            }
            if (too_big)
            {
                p = (now - 1e-4 <= was);
            }
            if (rand() / (RAND_MAX + 1.0) < p)
            {
                curr_node = result;
            }
        }
    }
    return curr_node;
}

void temp_to_rgb(double T, unsigned char *r, unsigned char *g, unsigned char *b){
    if(T<100) T = 100; if(T>40000) T = 40000;
    double R = 0, G = 0, B = 0, t;
    if(T <= 2000){ R = 255; G = (T - 100) / (2000 - 100) * 100; B = 0; }
    else if(T <= 4000){ t = (T - 2000) / 2000; R = 255; G = 100 + t*155; B = t * 80; }
    else if(T <= 6500){ t = (T - 4000) / 2500; R = 255 - t*10; G = 255 - t*5; B = 80 + t*175; }
    else{ t = (T <= 20000 ? (T - 6500) / (20000 - 6500) : 1); R = 245 - t*200; G = 250 - t*150; B = 255; }
    *r = R; *g = G; *b = B;
}


void node_to_xy(struct node_t *node, double hot, double *rx, double *ry)
{
    if (hot < 0.0)
    {
        hot = 0.0;
    } 
    
    double x, y;

    x = 1.0 / (1.0 + cost(node) / 100.0);
    y = 1.0 / (1.0 + hot / 100.0);

    *rx = x;
    *ry = y;
}

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font* font;

void paint_remove_node(struct node_t *node, double hot, int W, int H)
{
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    double x, y;
    node_to_xy(node, hot, &x, &y);
    SDL_RenderDrawPoint(renderer, x * W, y * H);
}

void paint_update_node(struct node_t *node, double hot, int W, int H)
{
    unsigned char r, g, b;
    temp_to_rgb(hot * 14.0, &r, &g, &b);
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    double x, y;
    node_to_xy(node, hot, &x, &y);
    SDL_RenderDrawPoint(renderer, x * W, y * H);
}


void draw_text(int x, int y, char *text)
{
    SDL_Color White = {255, 255, 255};

    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, text, White); 

    SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect Message_rect;
    Message_rect.x = x;
    Message_rect.y = y - surfaceMessage->h;
    Message_rect.w = surfaceMessage->w;
    Message_rect.h = surfaceMessage->h;

    SDL_RenderCopy(renderer, Message, NULL, &Message_rect);

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(Message);
}




#define PARALLEL
struct node_t *optimize_tree(struct node_t *node, int N, int K, int interactive)
{
    tree_map.clear();
    hotspot_map.clear();
    tree_parent.clear();
    used.clear();
    
    /* not clear hash map, becouse node can have already calculated hash after older operations */
    // hash_map.clear();

    double T0 = 400.0;

    tree_map[cost(node)] = node;
    hotspot_map[T0] = node;
    tree_parent[node] = node;
    tree_depth[node] = 0;

    int W = 900, H = 900;
    SDL_Event event;

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();

    if (SDL_CreateWindowAndRenderer(W, H, 0, &window, &renderer) != 0) {
        return node;
    }
    
    font = TTF_OpenFont("font.ttf", 16);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        double x, y;
        node_to_xy(node, T0, &x, &y);
        SDL_Rect rect = {x * W - 1, y * H - 1, 3, 3};
        SDL_RenderDrawRect(renderer, &rect);
    }

    /* go through tree, apply randomly optimizations */
    int tim = 0;
    #ifdef PARALLEL
    #pragma omp parallel for schedule(dynamic) default(shared)
    #endif
    for (int i = 0; i < N; ++i)
    {
        if (omp_get_thread_num() == 0)
        {
            while (SDL_PollEvent(&event)) {}
            
            if (SDL_GetTicks() > tim)
            {
                tim = SDL_GetTicks() + 16;
                SDL_RenderPresent(renderer);
            }
            printf("i=%d, %d nodes [by thread %d]\n", i, hotspot_map.size(), omp_get_thread_num());
        }

        struct node_t *cur_node = NULL;
        double cur_cost;
        double cur_hot;
        double cur_new_hot;

        #ifdef PARALLEL
        #pragma omp critical (ResourceA) 
        #endif
        {
            if (hotspot_map.size() != 0)
            {
                /* select random node with x^2 prob */
                double partition = rand() / (RAND_MAX + 1.0);
                partition *= partition;
                partition *= (double)(N - i) / N;
                size_t index = partition * hotspot_map.size();

                auto x = hotspot_map.find_by_order(index);
                cur_node = x->second;
                cur_hot = x->first;

                paint_remove_node(cur_node, cur_hot, W, H);
                hotspot_map.erase(hotspot_map.find_by_order(index));
            }
            else
            {
                cur_node = node;
                cur_hot = T0;
            }
        }

        
        cur_cost = cost(cur_node);
        cur_new_hot = cur_hot / (1.0 + cur_cost / 350.0);
        
        /* mutate node some times */
        int bad_nodes = 0;
        for (int t = 0; t < (i < 50 ? 1000 : K); ++t)
        {
            struct node_t *res_node;
            res_node = mutate_tree_inner(cur_node, i / (double)N, cur_cost > 5000.0);
            double res_cost = cost(res_node);
            

            /* is node used? */
            #ifdef PARALLEL
            #pragma omp critical (ResourceA) 
            #endif
            {
                if (used.find(tree_hash(res_node)) != used.end())
                {
                    /* decrease node value */
                    if (cur_node != tree_parent[used[tree_hash(res_node)]])
                    {
                        // cur_new_hot *= 0.95;
                        bad_nodes++;
                    }

                    if (tree_depth[cur_node] + 1 < tree_depth[used[tree_hash(res_node)]])
                    {
                        tree_depth[used[tree_hash(res_node)]] = tree_depth[cur_node] + 1;
                        tree_parent[used[tree_hash(res_node)]] = cur_node;
                    }
                }
                else
                {
                    /* add new node */
                    double delta = cur_cost - res_cost;
                    double res_hot = cur_hot * 0.995 + 20.0 * delta; // * (1 + 0.3 * (res_cost < cur_cost));

                    if (res_cost - 1e-6 > cur_cost)
                    {
                        // was_better = 1;
                        bad_nodes++;
                    }
                    else
                    {
                        bad_nodes--;
                    }
                    
                    hotspot_map.insert({res_hot, res_node});
                    paint_update_node(res_node, res_hot, W, H);
                    tree_map.insert({res_cost, res_node});
                    used[tree_hash(res_node)] = res_node;

                    tree_parent[res_node] = cur_node;
                    tree_depth[res_node] = tree_depth[cur_node] + 1;
                }
            }
        }

        #ifdef PARALLEL
        #pragma omp critical (ResourceA)
        #endif
        {
            if (bad_nodes > 0)
            {
                /* decrease node value */
                cur_new_hot *= 0.95;
            }
            
            hotspot_map.insert({cur_new_hot, cur_node});        
            paint_update_node(cur_node, cur_new_hot, W, H);
        }
    }

    printf("WAITING TO PRESS\n");

    SDL_FPoint *points = malloc(sizeof(*points) * hotspot_map.size());
    std::map<struct node_t *, double> hots;
    {
        int id = 0;
        for (auto &t : hotspot_map)
        {
            hots[t.second] = t.first;
            double x, y; node_to_xy(t.second, t.first, &x, &y);
            points[id].x = x * W; points[id].y = y * H;
            id++;
        }
    }
    while (interactive) 
    {
        if (SDL_PollEvent(&event)){
            if (event.type == SDL_MOUSEBUTTONDOWN){break;} 
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawPointsF(renderer, points, hotspot_map.size());
        
        int mouseX, mouseY, selX, selY;
        Uint32 buttonState = SDL_GetMouseState(&mouseX, &mouseY);
        double dmin = INFINITY, sel_h;
        struct node_t *sel;
        for (auto &t : hotspot_map)
        {
            double x, y; node_to_xy(t.second, t.first, &x, &y);
            double d = (x * W - mouseX) * (x * W - mouseX) + (y * H - mouseY) * (y * H - mouseY);
            if (d < dmin) { dmin = d; sel_h = t.first; sel = t.second; selX = x * W; selY = y * H; }
        }
        if (sel)
        {
            char buf[1024 * 32];
            char *text = export_basic(buf, sel);
            *text = 0;
            draw_text(selX, selY, buf);
            SDL_RenderPresent(renderer);
        }
        SDL_Delay(10);
    }
            
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("RESULT TREE: score=%g\n", tree_map.begin()->first);

    /* print path to result */

    printf("Key optimization moves: ----------------- \n");
    {
        std::vector<struct node_t *> path(1, tree_map.begin()->second);
        while (tree_parent[path.back()] != path.back())
        {
            path.push_back(tree_parent[path.back()]);
        }
        std::reverse(path.begin(), path.end());

        char buf[1024 * 32];
        int id = 0;
        for (auto v : path)
        {
            char *text = export_basic(buf, v);
            *text = 0;
            printf("%d.   %s\n", id, buf);
            id++;
        }
    }
    
    return tree_map.begin()->second;
}

