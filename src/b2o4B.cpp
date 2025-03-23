#include <windows.h>

#include <SFML/Graphics.hpp>

#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <mutex>

namespace Renderer
{
    sf::Texture tiles[12];
    sf::Font font;

    std::thread thread;

    std::mutex gbMutex;
}

struct Transform
{
    int x, y, c;
    inline int operator()(int i, int j)
    {
        return i * x + j * y + c;
    }
};

template <typename T>
struct VecView
{
    std::vector<std::vector<T>> *vec;

    Transform x, y;

    inline T &operator()(int i, int j)
    {
        return vec->operator[](x(i, j))[y(i, j)];
    }
};

class GameBoard
{
public:
    const int size = 4;
    std::vector<std::vector<int>> gb;

    std::mt19937 rnd;

    int points = 0;
    bool end = 0;

    std::string hb = "Yoxi!";

    GameBoard(int sz = 4) : size(sz) // actually, can't change yet.
    {
        rnd = std::mt19937(std::chrono::system_clock::now().time_since_epoch().count());
        gb = std::vector<std::vector<int>>(sz, std::vector<int>(sz, 0));

        gen1();
        gen1();
    }

    int remain()
    {
        int cnt = 0;
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                if (!gb[i][j])
                {
                    cnt++;
                }
            }
        }
        return cnt;
    }

    bool gen1()
    {
        if (!remain())
        {
            return false;
        }
        int i = rnd() % size, j = rnd() % size;
        while (gb[i][j])
        {
            i = rnd() % size, j = rnd() % size;
        }
        gb[i][j] = !(rnd() % 5) + 1;
        return true;
    }

    bool checkdie()
    {
        if (remain())
        {
            return false;
        }
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                if (i < size - 1 && gb[i][j] == gb[i + 1][j])
                {
                    return false;
                }
                if (j < size - 1 && gb[i][j] == gb[i][j + 1])
                {
                    return false;
                }
            }
        }
        hb = "You lost! /hanx";
        return end = true;
    }

    void move(Transform x, Transform y)
    {
        if (end)
        {
            return;
        }
        std::lock_guard guard(Renderer::gbMutex);
        // doesn't change x, y -> min
        VecView<int> view{&gb, x, y};
        auto ogb = gb;
        int mxcrafted = 0;
        for (int i = 0; i < size; i++)
        {
            int las = 0;
            for (int j = 0; j < size; j++)
            {
                if (view(i, j))
                {
                    view(i, las++) = view(i, j);
                }
            }
            for (int j = las; j < size; j++)
            {
                view(i, j) = 0;
            }
            for (int j = 0; j < size - 1; j++)
            {
                if (view(i, j) == view(i, j + 1) && view(i, j))
                {
                    view(i, j)++;
                    points += 1 << view(i, j);
                    mxcrafted = std::max(mxcrafted, view(i, j));
                    view(i, ++j) = 0;
                }
            }
            las = 0;
            for (int j = 0; j < size; j++)
            {
                if (view(i, j))
                {
                    view(i, las++) = view(i, j);
                }
            }
            for (int j = las; j < size; j++)
            {
                view(i, j) = 0;
            }
        }
        if (ogb == gb)
        {
            return; // doesn't change
        }
        if (mxcrafted)
        {
            hb = "You crafted a " + std::to_string(1 << mxcrafted) + "! Yo!";
        }
        else
        {
            hb = "Yoxi!";
        }
        if (mxcrafted == 11)
        {
            hb = "You win! /ll";
            end = true;
            return;
        }
        if (remain())
        {
            gen1();
        }
        checkdie();
    }
} gb;

namespace Renderer
{
    void init()
    {
        if (!font.openFromFile("CONSOLA.ttf"))
        {
            std::cout << "Error: Cannot find Consolas." << std::endl;
            exit(0);
        }
        for (int i = 0; i < 12; i++)
        {
            if (!tiles[i].loadFromFile("./pics/" + std::to_string(1 << i) + ".png"))
            {
                std::cout << "Error: Cannot find pics." << std::endl;
                exit(0);
            }
            std::get<0>(std::tie(i));
        }
    }

    std::string bobify(std::string s)
    {
        std::string t = "";
        for (char c : s)
        {
            if (c == '0')
            {
                t += 'o';
            }
            else if (c == '6')
            {
                t += 'b';
            }
            else if (c == '8')
            {
                t += 'B';
            }
            else
            {
                t += c;
            }
        }
        return t;
    }

    std::string bobify(int v)
    {
        return bobify(std::to_string(v));
    }

    void render(sf::RenderWindow *window)
    {
        if (!window->setActive(true))
        {
            exit(0);
        }

        while (true)
        {
            if (window->isOpen())
            {
                window->clear(sf::Color(0xFC, 0xFC, 0xD5));

                sf::Text title(font, "b2o4B");
                title.setCharacterSize(50);
                title.setFillColor(sf::Color::Black);
                title.setStyle(sf::Text::Bold);
                float width = title.getLocalBounds().size.x;
                title.setPosition({500.f - width / 2, 0.f});
                window->draw(title);

                std::lock_guard gbGuard(gbMutex);

                sf::Text points(font, "Points: " + bobify(gb.points));
                points.setCharacterSize(30);
                points.setFillColor(sf::Color::Black);
                points.setPosition({50.f, 70.f});
                window->draw(points);

                sf::Text hb(font, "Happybob: " + bobify(gb.hb));
                hb.setCharacterSize(30);
                hb.setFillColor(sf::Color::Black);
                hb.setPosition({350.f, 70.f});
                window->draw(hb);

                sf::RectangleShape table({900.f, 900.f});
                table.setFillColor(sf::Color(248, 248, 170));
                table.setPosition({50.f, 150.f});
                window->draw(table);

                for (int i = 0; i < gb.size; i++)
                {
                    for (int j = 0; j < gb.size; j++)
                    {
                        sf::Sprite tile(tiles[gb.gb[i][j]]);
                        tile.setPosition({70.f + i * 220.f, 170.f + j * 220.f});
                        if (!gb.gb[i][j])
                        {
                            tile.setColor(sf::Color(255, 255, 255, 128));
                        }
                        window->draw(tile);

                        if (gb.gb[i][j])
                        {
                            sf::Text tiln(font, bobify(1 << gb.gb[i][j]));
                            tiln.setCharacterSize(30);
                            tiln.setFillColor(sf::Color::Black);
                            tiln.setPosition({80.f + i * 220.f, 180.f + j * 220.f});
                            window->draw(tiln);
                        }
                    }
                }

                window->display();
            }
            else
            {
                break;
            }
        }
    }

    std::thread &start(sf::RenderWindow *window)
    {
        return thread = std::thread(render, window);
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode({1000, 1100}), "b2o4B");
    window.setFramerateLimit(60);

    // window.setVerticalSyncEnabled(true);
    // sf::WindowHandle handle = window.getNativeHandle();
    // SetWindowPos(handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    if (!window.setActive(false))
    {
        return 0;
    }

    Renderer::init();
    Renderer::start(&window);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const sf::Event::KeyPressed *key = event->getIf<sf::Event::KeyPressed>())
            {
                std::cout << static_cast<int>(key->scancode) << std::endl;
                switch (key->scancode)
                {
                case sf::Keyboard::Scan::Up:
                case sf::Keyboard::Scan::W:
                    // gb.up();
                    gb.move({1, 0, 0}, {0, 1, 0});
                    break;

                case sf::Keyboard::Scan::Down:
                case sf::Keyboard::Scan::S:
                    // gb.down();
                    gb.move({1, 0, 0}, {0, -1, gb.size - 1});
                    break;

                case sf::Keyboard::Scan::Left:
                case sf::Keyboard::Scan::A:
                    // gb.left();
                    gb.move({0, 1, 0}, {1, 0, 0});
                    break;

                case sf::Keyboard::Scan::Right:
                case sf::Keyboard::Scan::D:
                    // gb.right();
                    gb.move({0, -1, gb.size - 1}, {1, 0, 0});
                    break;
                }
            }
        }
    }

    Renderer::thread.join();

    return 0;
}
