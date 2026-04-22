FROM ubuntu:22.04 AS builder

# DEBIAN_FRONTEND=noninteractive — apt не будет задавать интерактивные вопросы.
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
	build-essential \
	cmake \
	git \
	ca-certificates \
	libboost-system-dev \
	libssl-dev \
	libcurl4-openssl-dev \
	nlohmann-json3-dev \
	zlib1g-dev \
	&& rm -rf /var/lib/apt/lists/*

# Собираем tgbot-cpp из исходников
# --depth=1 — качаем только последний коммит, экономим трафик и место.
WORKDIR /tmp
RUN git clone --depth=1 https://github.com/reo7sp/tgbot-cpp.git \
	&& cd tgbot-cpp \
	&& cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
	&& cmake --build build --parallel "$(nproc)" \
	&& cmake --install build \
	&& cd .. && rm -rf tgbot-cpp

# Копируем исходники бота.
WORKDIR /app
COPY CMakeLists.txt ./
COPY include/       ./include/
COPY src/           ./src/

# Компилируем в Release.
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
	&& cmake --build build --parallel "$(nproc)"

FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
	libboost-system1.74.0 \
	libssl3 \
	libcurl4 \
	zlib1g \
	ca-certificates \
	&& rm -rf /var/lib/apt/lists/*

RUN useradd --system --create-home --uid 1000 bot
USER bot
WORKDIR /home/bot

# Копируем готовый бинарник из builder-стадии.
COPY --from=builder /app/build/currency_bot /usr/local/bin/currency_bot
# И динамически подгружаемую библиотеку tgbot-cpp
COPY --from=builder /usr/local/lib/libTgBot.* /usr/local/lib/

# Обновляем кэш динамического линкёра, чтобы libTgBot был найден.
# Запускаем от root на короткий момент, потом возвращаемся на bot.
USER root
RUN ldconfig
USER bot

CMD ["currency_bot"]