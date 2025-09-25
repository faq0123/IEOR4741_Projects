Our trading strategy is based on detecting short-term momentum in price movements.
Specifically, we monitor the three most recent price points and check whether they are moving strictly upward or strictly downward.
If prices form a sequence of three consecutive increases or decreases, this indicates a short-term momentum trend.

To avoid reacting to small fluctuations or noise, we also require that the cumulative percentage change between
the first and the last of the three prices exceeds a predefined threshold (e.g., ±0.5%).
A buy signal is generated if the prices show strictly increasing behavior and the cumulative gain is sufficiently large.
Similarly, a sell signal is generated if the prices show strictly decreasing behavior and the cumulative loss is sufficiently large.
In all other cases, the strategy holds its position and waits for a clearer momentum signal.
