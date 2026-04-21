"use client";

import { useState } from "react";
import { motion, AnimatePresence } from "framer-motion";
import { Button } from "./ui/button";
import { Card } from "./ui/card";
import { Sparkles, RefreshCw } from "lucide-react";

interface Fortune {
  quote: string;
  numbers: number[];
}

const fortunes: Fortune[] = [
  {
    quote:
      "Your future is created by what you do today, not tomorrow.",
    numbers: [7, 14, 23, 31, 42, 56],
  },
  {
    quote:
      "The best time to plant a tree was 20 years ago. The second best time is now.",
    numbers: [3, 18, 27, 35, 49, 63],
  },
  {
    quote:
      "Success is not final, failure is not fatal: it is the courage to continue that counts.",
    numbers: [9, 16, 24, 38, 47, 55],
  },
  {
    quote: "Believe you can and you're halfway there.",
    numbers: [2, 11, 29, 33, 44, 51],
  },
  {
    quote:
      "The only way to do great work is to love what you do.",
    numbers: [5, 12, 21, 36, 43, 58],
  },
  {
    quote:
      "Life is what happens to you while you're busy making other plans.",
    numbers: [1, 19, 26, 34, 41, 62],
  },
  {
    quote:
      "It does not matter how slowly you go as long as you do not stop.",
    numbers: [8, 15, 22, 37, 46, 59],
  },
  {
    quote: "In the middle of difficulty lies opportunity.",
    numbers: [4, 13, 25, 32, 48, 57],
  },
  {
    quote: "The journey of a thousand miles begins with one step.",
    numbers: [6, 17, 28, 39, 45, 61],
  },
  {
    quote: "What lies behind us and what lies before us are tiny matters compared to what lies within us.",
    numbers: [10, 20, 30, 40, 50, 60],
  },
  {
    quote: "Be yourself; everyone else is already taken.",
    numbers: [12, 24, 36, 41, 53, 65],
  },
  {
    quote: "Two roads diverged in a wood, and I took the one less traveled by, and that has made all the difference.",
    numbers: [14, 28, 35, 49, 56, 63],
  },
  {
    quote: "The mind is everything. What you think you become.",
    numbers: [1, 8, 15, 22, 29, 36],
  },
  {
    quote: "Whether you think you can or you think you can't, you're right.",
    numbers: [3, 9, 18, 27, 45, 54],
  },
  {
    quote: "Yesterday is history, tomorrow is a mystery, today is a gift of God, which is why we call it the present.",
    numbers: [7, 21, 28, 42, 49, 56],
  },
  {
    quote: "Don't judge each day by the harvest you reap but by the seeds that you plant.",
    numbers: [11, 22, 33, 44, 55, 66],
  },
  {
    quote: "The only impossible journey is the one you never begin.",
    numbers: [2, 13, 24, 35, 46, 57],
  },
  {
    quote: "In the end, we will remember not the words of our enemies, but the silence of our friends.",
    numbers: [16, 32, 48, 64, 17, 33],
  },
  {
    quote: "It is during our darkest moments that we must focus to see the light.",
    numbers: [5, 10, 25, 40, 55, 65],
  },
  {
    quote: "Spread love everywhere you go. Let no one ever come to you without leaving happier.",
    numbers: [9, 18, 36, 45, 54, 63],
  },
  {
    quote: "When you reach the end of your rope, tie a knot in it and hang on.",
    numbers: [4, 8, 16, 32, 48, 52],
  },
  {
    quote: "Don't let what you cannot do interfere with what you can do.",
    numbers: [6, 12, 24, 30, 42, 60],
  },
  {
    quote: "If you want to lift yourself up, lift up someone else.",
    numbers: [15, 30, 45, 52, 67, 3],
  },
  {
    quote: "The future belongs to those who believe in the beauty of their dreams.",
    numbers: [19, 38, 57, 4, 23, 68],
  },
  {
    quote: "It is better to fail in originality than to succeed in imitation.",
    numbers: [23, 46, 69, 8, 17, 54],
  },
  {
    quote: "The way to get started is to quit talking and begin doing.",
    numbers: [27, 54, 12, 41, 65, 29],
  },
  {
    quote: "Don't be afraid to give up the good to go for the great.",
    numbers: [31, 62, 18, 47, 6, 35],
  },
  {
    quote: "Innovation distinguishes between a leader and a follower.",
    numbers: [35, 70, 14, 53, 9, 26],
  },
  {
    quote: "The greatest glory in living lies not in never falling, but in rising every time we fall.",
    numbers: [39, 7, 28, 56, 11, 64],
  },
  {
    quote: "Your time is limited, don't waste it living someone else's life.",
    numbers: [43, 16, 32, 59, 5, 48],
  },
  {
    quote: "If life were predictable it would cease to be life, and be without flavor.",
    numbers: [47, 24, 61, 13, 37, 52],
  },
  {
    quote: "Life is really simple, but we insist on making it complicated.",
    numbers: [51, 2, 39, 66, 21, 44],
  },
  {
    quote: "You have within you right now, everything you need to deal with whatever the world can throw at you.",
    numbers: [55, 10, 43, 69, 25, 38],
  },
  {
    quote: "Believe in yourself and all that you are. Know that there is something inside you that is greater than any obstacle.",
    numbers: [59, 18, 47, 3, 29, 65],
  },
  {
    quote: "The only person you are destined to become is the person you decide to be.",
    numbers: [63, 26, 51, 7, 33, 58],
  },
  {
    quote: "Go confidently in the direction of your dreams. Live the life you have imagined.",
    numbers: [67, 34, 55, 11, 37, 62],
  },
  {
    quote: "When one door of happiness closes, another opens, but we often look so long at the closed door that we do not see the one that has been opened for us.",
    numbers: [4, 42, 59, 15, 68, 27],
  },
  {
    quote: "Everything you've ever wanted is on the other side of fear.",
    numbers: [8, 46, 63, 19, 35, 52],
  },
  {
    quote: "Dream big and dare to fail.",
    numbers: [12, 50, 67, 23, 39, 56],
  },
  {
    quote: "You miss 100% of the shots you don't take.",
    numbers: [16, 54, 2, 27, 43, 60],
  }
];

type CookieState = "unopened" | "cracking" | "opened";

export function FortuneCookie() {
  const [state, setState] = useState<CookieState>("unopened");
  const [currentFortune, setCurrentFortune] =
    useState<Fortune | null>(null);

  const crackCookie = () => {
    if (state !== "unopened") return;

    setState("cracking");
    const randomFortune =
      fortunes[Math.floor(Math.random() * fortunes.length)];
    setCurrentFortune(randomFortune);

    // Show cracking animation for 2 seconds, then reveal fortune
    setTimeout(() => {
      setState("opened");
    }, 2000);
  };

  const getNewCookie = () => {
    setState("unopened");
    setCurrentFortune(null);
  };

  return (
    <div className="flex flex-col items-center justify-center min-h-screen p-6 bg-gradient-to-br from-orange-50/80 to-amber-100/80 backdrop-blur-sm">
      <AnimatePresence mode="wait">
        {state === "unopened" && (
          <motion.div
            key="unopened"
            initial={{ scale: 0, rotate: -180 }}
            animate={{ scale: 1, rotate: 0 }}
            exit={{ scale: 0.8, opacity: 0 }}
            transition={{
              type: "spring",
              stiffness: 260,
              damping: 20,
              duration: 0.6,
            }}
            className="flex flex-col items-center"
          >
            <motion.div
              whileHover={{ scale: 1.05 }}
              whileTap={{ scale: 0.95 }}
              animate={{
                y: [0, -10, 0],
                rotate: [0, 2, -2, 0],
              }}
              transition={{
                y: {
                  duration: 2,
                  repeat: Infinity,
                  ease: "easeInOut",
                },
                rotate: {
                  duration: 4,
                  repeat: Infinity,
                  ease: "easeInOut",
                },
              }}
              onClick={crackCookie}
              className="cursor-pointer mb-8"
            >
              <div className="relative">
                {/* Cookie Shadow */}
                <div className="absolute top-2 left-2 w-32 h-20 bg-black/20 rounded-full blur-md" />

                {/* Fortune Cookie */}
                <div className="relative w-32 h-20 bg-gradient-to-br from-yellow-200 to-amber-300 rounded-full transform rotate-12 shadow-lg border-2 border-amber-400">
                  {/* Cookie texture lines */}
                  <div className="absolute inset-2 border border-amber-500/30 rounded-full" />
                  <div className="absolute inset-4 border border-amber-500/20 rounded-full" />

                  {/* Sparkle effects */}
                  <motion.div
                    animate={{ rotate: 360 }}
                    transition={{
                      duration: 8,
                      repeat: Infinity,
                      ease: "linear",
                    }}
                    className="absolute -top-2 -right-2"
                  >
                    <Sparkles className="w-6 h-6 text-yellow-400 drop-shadow-lg" />
                  </motion.div>
                  <motion.div
                    animate={{ rotate: -360 }}
                    transition={{
                      duration: 6,
                      repeat: Infinity,
                      ease: "linear",
                    }}
                    className="absolute -bottom-1 -left-2"
                  >
                    <Sparkles className="w-4 h-4 text-orange-400 drop-shadow-lg" />
                  </motion.div>
                </div>
              </div>
            </motion.div>

            <motion.div
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: 0.5 }}
              className="text-center relative"
            >
              {/* Magical aura around title */}
              <motion.div
                initial={{ scale: 0, opacity: 0 }}
                animate={{ scale: 1, opacity: 0.2 }}
                transition={{ delay: 0.8, duration: 1.2 }}
                className="absolute inset-0 bg-gradient-to-r from-amber-200/30 via-yellow-200/30 to-orange-200/30 rounded-full blur-2xl transform scale-150"
              />

              {/* Orbiting sparkles around title */}
              {[...Array(8)].map((_, i) => (
                <motion.div
                  key={i}
                  initial={{ scale: 0, opacity: 0 }}
                  animate={{
                    scale: [0, 1, 0.8, 1],
                    opacity: [0, 0.8, 0.6, 0.8],
                    rotate: 360,
                  }}
                  transition={{
                    scale: {
                      delay: 1 + i * 0.1,
                      duration: 0.8,
                    },
                    opacity: {
                      delay: 1 + i * 0.1,
                      duration: 0.8,
                    },
                    rotate: {
                      delay: 1.5,
                      duration: 8,
                      repeat: Infinity,
                      ease: "linear",
                    },
                  }}
                  className="absolute"
                  style={{
                    left: `${50 + Math.cos((i * Math.PI * 2) / 8) * 80}%`,
                    top: `${50 + Math.sin((i * Math.PI * 2) / 8) * 60}%`,
                    transform: "translate(-50%, -50%)",
                  }}
                >
                  <Sparkles
                    className={`w-${i % 2 === 0 ? "3" : "2"} h-${i % 2 === 0 ? "3" : "2"} text-${i % 3 === 0 ? "yellow" : i % 3 === 1 ? "amber" : "orange"}-400/70`}
                  />
                </motion.div>
              ))}

              {/* Enhanced title with gradient and glow */}
              <motion.h1
                initial={{ scale: 0.8, opacity: 0 }}
                animate={{ scale: 1, opacity: 1 }}
                transition={{
                  delay: 0.7,
                  duration: 0.8,
                  ease: "backOut",
                }}
                className="text-3xl mb-3 bg-gradient-to-r from-amber-700 via-yellow-600 to-orange-700 bg-clip-text text-transparent relative z-10"
                style={{
                  filter:
                    "drop-shadow(0 2px 8px rgba(251, 191, 36, 0.4))",
                  textShadow:
                    "0 0 30px rgba(251, 191, 36, 0.6)",
                }}
              >
                <motion.span
                  animate={{
                    scale: [1, 1.02, 1],
                  }}
                  transition={{
                    duration: 4,
                    repeat: Infinity,
                    ease: "easeInOut",
                  }}
                >
                  Fortune Cookie
                </motion.span>
              </motion.h1>

              {/* Enhanced subtitle with shimmer effect */}
              <motion.p
                initial={{ opacity: 0, y: 10 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: 1.2 }}
                className="text-amber-700 mb-4 relative z-10"
                style={{
                  textShadow: "0 1px 3px rgba(0,0,0,0.1)",
                }}
              >
                <motion.span className="bg-clip-text">
                  Tap the cookie to crack it open!
                </motion.span>
              </motion.p>

              {/* Enhanced magic text with pulsing border */}
              <motion.div
                initial={{ opacity: 0, scale: 0.9 }}
                animate={{ opacity: 1, scale: 1 }}
                transition={{ delay: 1.5 }}
                className="relative z-10"
              >
                <motion.div
                  animate={{
                    scale: [1, 1.05, 1],
                    boxShadow: [
                      "0 0 0 1px rgba(251, 191, 36, 0.2)",
                      "0 0 0 2px rgba(251, 191, 36, 0.4)",
                      "0 0 0 1px rgba(251, 191, 36, 0.2)",
                    ],
                  }}
                  transition={{
                    duration: 2,
                    repeat: Infinity,
                    ease: "easeInOut",
                  }}
                  className="inline-flex items-center justify-center gap-2 px-4 py-2 rounded-full bg-gradient-to-r from-amber-50/80 to-yellow-50/80 backdrop-blur-sm border border-amber-200/50"
                >
                  <motion.div
                    animate={{
                      rotate: 360,
                      scale: [1, 1.2, 1],
                    }}
                    transition={{
                      rotate: {
                        duration: 4,
                        repeat: Infinity,
                        ease: "linear",
                      },
                      scale: {
                        duration: 2,
                        repeat: Infinity,
                        ease: "easeInOut",
                      },
                    }}
                  >
                    <Sparkles className="w-4 h-4 text-amber-500" />
                  </motion.div>
                  <span className="text-sm text-amber-700 font-medium">
                    Magic awaits inside
                  </span>
                  <motion.div
                    animate={{
                      rotate: -360,
                      scale: [1, 1.2, 1],
                    }}
                    transition={{
                      rotate: {
                        duration: 3,
                        repeat: Infinity,
                        ease: "linear",
                      },
                      scale: {
                        duration: 2,
                        repeat: Infinity,
                        ease: "easeInOut",
                        delay: 0.5,
                      },
                    }}
                  >
                    <Sparkles className="w-4 h-4 text-orange-500" />
                  </motion.div>
                </motion.div>
              </motion.div>
            </motion.div>
          </motion.div>
        )}

        {state === "cracking" && (
          <motion.div
            key="cracking"
            initial={{ scale: 1 }}
            animate={{ scale: [1, 1.2, 0.8, 1.1] }}
            transition={{ duration: 2, ease: "easeInOut" }}
            className="flex flex-col items-center"
          >
            <motion.div
              animate={{
                rotate: [0, 5, -5, 10, -10, 0],
                scale: [1, 1.1, 0.9, 1.05, 0.95, 1],
              }}
              transition={{ duration: 2, ease: "easeInOut" }}
              className="relative mb-8"
            >
              {/* Cracking cookie with split effect */}
              <div className="relative w-32 h-20">
                {/* Left half */}
                <motion.div
                  animate={{ x: [-16, -24], rotate: [0, -15] }}
                  transition={{ duration: 2, ease: "easeOut" }}
                  className="absolute left-0 w-16 h-20 bg-gradient-to-br from-yellow-200 to-amber-300 rounded-l-full border-2 border-amber-400 overflow-hidden"
                >
                  <div className="absolute inset-1 border border-amber-500/30 rounded-l-full" />
                </motion.div>

                {/* Right half */}
                <motion.div
                  animate={{ x: [16, 24], rotate: [0, 15] }}
                  transition={{ duration: 2, ease: "easeOut" }}
                  className="absolute right-0 w-16 h-20 bg-gradient-to-br from-yellow-200 to-amber-300 rounded-r-full border-2 border-amber-400 overflow-hidden"
                >
                  <div className="absolute inset-1 border border-amber-500/30 rounded-r-full" />
                </motion.div>

                {/* Cracking particles */}
                {[...Array(8)].map((_, i) => (
                  <motion.div
                    key={i}
                    initial={{ opacity: 0, scale: 0 }}
                    animate={{
                      opacity: [0, 1, 0],
                      scale: [0, 1, 0],
                      x: [0, (Math.random() - 0.5) * 100],
                      y: [0, (Math.random() - 0.5) * 100],
                    }}
                    transition={{
                      duration: 1.5,
                      delay: i * 0.1,
                      ease: "easeOut",
                    }}
                    className="absolute top-1/2 left-1/2 w-2 h-2 bg-amber-300 rounded-full"
                  />
                ))}
              </div>
            </motion.div>

            <motion.p
              initial={{ opacity: 0 }}
              animate={{ opacity: [0, 1, 0.7, 1] }}
              transition={{ duration: 2 }}
              className="text-amber-700 text-center"
            >
              Cracking open your fortune...
            </motion.p>
          </motion.div>
        )}

        {state === "opened" && currentFortune && (
          <motion.div
            key="opened"
            initial={{ scale: 0, opacity: 0 }}
            animate={{ scale: 1, opacity: 1 }}
            transition={{
              type: "spring",
              stiffness: 300,
              damping: 30,
              delay: 0.2,
            }}
            className="w-full max-w-md"
          >
            <Card className="p-6 bg-white/90 backdrop-blur-sm border-amber-200 shadow-xl">
              <motion.div
                initial={{ y: 20, opacity: 0 }}
                animate={{ y: 0, opacity: 1 }}
                transition={{ delay: 0.4 }}
                className="text-center mb-6 relative"
              >
                {/* Magical glow background */}
                <motion.div
                  initial={{ scale: 0, opacity: 0 }}
                  animate={{ scale: 1, opacity: 0.3 }}
                  transition={{ delay: 0.6, duration: 1 }}
                  className="absolute inset-0 bg-gradient-to-r from-amber-200/50 via-yellow-200/50 to-orange-200/50 rounded-full blur-xl"
                />

                {/* Floating sparkles */}
                {[...Array(6)].map((_, i) => (
                  <motion.div
                    key={i}
                    initial={{ scale: 0, opacity: 0 }}
                    animate={{
                      scale: [0, 1, 0.8, 1],
                      opacity: [0, 1, 0.7, 1],
                      y: [0, -20, 0, -15, 0],
                      x: [
                        0,
                        Math.sin(i) * 20,
                        0,
                        -Math.sin(i) * 15,
                        0,
                      ],
                    }}
                    transition={{
                      delay: 0.8 + i * 0.1,
                      duration: 3,
                      repeat: Infinity,
                      ease: "easeInOut",
                    }}
                    className="absolute"
                    style={{
                      left: `${20 + i * 12}%`,
                      top: `${10 + (i % 2) * 20}%`,
                    }}
                  >
                    <Sparkles
                      className={`w-${i % 2 === 0 ? "4" : "3"} h-${i % 2 === 0 ? "4" : "3"} text-${i % 3 === 0 ? "yellow" : i % 3 === 1 ? "amber" : "orange"}-400 drop-shadow-lg`}
                    />
                  </motion.div>
                ))}

                {/* Main sparkle with complex animation */}
                <motion.div
                  initial={{ scale: 0, rotate: 0 }}
                  animate={{
                    scale: [0, 1.2, 1],
                    rotate: 360,
                  }}
                  transition={{
                    scale: {
                      delay: 0.5,
                      duration: 0.8,
                      ease: "backOut",
                    },
                    rotate: {
                      delay: 0.7,
                      duration: 3,
                      repeat: Infinity,
                      ease: "linear",
                    },
                  }}
                  className="inline-block mb-4 relative z-10"
                >
                  <motion.div
                    animate={{
                      scale: [1, 1.3, 1],
                      filter: [
                        "brightness(1)",
                        "brightness(1.5)",
                        "brightness(1)",
                      ],
                    }}
                    transition={{
                      duration: 2,
                      repeat: Infinity,
                      ease: "easeInOut",
                    }}
                  >
                    <Sparkles className="w-10 h-10 text-amber-500 drop-shadow-2xl" />
                  </motion.div>

                  {/* Pulsing ring around main sparkle */}
                  <motion.div
                    initial={{ scale: 0, opacity: 0 }}
                    animate={{
                      scale: [0.8, 1.5, 0.8],
                      opacity: [0.8, 0, 0.8],
                    }}
                    transition={{
                      delay: 0.8,
                      duration: 2,
                      repeat: Infinity,
                      ease: "easeInOut",
                    }}
                    className="absolute inset-0 border-2 border-amber-400 rounded-full"
                  />
                </motion.div>

                {/* Enhanced title with gradient and glow */}
                <motion.h2
                  initial={{ scale: 0.8, opacity: 0 }}
                  animate={{ scale: 1, opacity: 1 }}
                  transition={{
                    delay: 0.7,
                    duration: 0.6,
                    ease: "backOut",
                  }}
                  className="text-2xl mb-4 bg-gradient-to-r from-amber-600 via-yellow-600 to-orange-600 bg-clip-text text-transparent relative z-10"
                  style={{
                    filter:
                      "drop-shadow(0 2px 4px rgba(251, 191, 36, 0.3))",
                    textShadow:
                      "0 0 20px rgba(251, 191, 36, 0.5)",
                  }}
                >
                  <motion.span
                    animate={{
                      scale: [1, 1.05, 1],
                    }}
                    transition={{
                      duration: 3,
                      repeat: Infinity,
                      ease: "easeInOut",
                    }}
                  >
                    Your Fortune
                  </motion.span>
                </motion.h2>
              </motion.div>

              <motion.blockquote
                initial={{ y: 20, opacity: 0 }}
                animate={{ y: 0, opacity: 1 }}
                transition={{ delay: 0.6 }}
                className="text-center text-gray-700 mb-6 italic leading-relaxed"
              >
                "{currentFortune.quote}"
              </motion.blockquote>

              <motion.div
                initial={{ y: 20, opacity: 0 }}
                animate={{ y: 0, opacity: 1 }}
                transition={{ delay: 0.8 }}
                className="text-center mb-6"
              >
                <h3 className="text-sm text-amber-700 mb-3">
                  Your Lucky Numbers
                </h3>
                <div className="flex justify-center gap-2 flex-wrap">
                  {currentFortune.numbers.map(
                    (number, index) => (
                      <motion.div
                        key={number}
                        initial={{ scale: 0 }}
                        animate={{ scale: 1 }}
                        transition={{
                          delay: 1 + index * 0.1,
                          type: "spring",
                          stiffness: 500,
                          damping: 25,
                        }}
                        className="w-10 h-10 bg-gradient-to-br from-amber-400 to-orange-500 rounded-full flex items-center justify-center shadow-md"
                      >
                        <span className="text-white font-medium text-sm">
                          {number}
                        </span>
                      </motion.div>
                    ),
                  )}
                </div>
              </motion.div>

              <motion.div
                initial={{ y: 20, opacity: 0 }}
                animate={{ y: 0, opacity: 1 }}
                transition={{ delay: 1.5 }}
                className="text-center"
              >
                <Button
                  onClick={getNewCookie}
                  className="bg-gradient-to-r from-amber-500 to-orange-500 hover:from-amber-600 hover:to-orange-600 text-white px-6 py-2 rounded-full shadow-lg transform transition-all duration-200 hover:scale-105"
                >
                  <RefreshCw className="w-4 h-4 mr-2" />
                  Get Another Fortune
                </Button>
              </motion.div>
            </Card>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}