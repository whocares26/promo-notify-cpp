#!/usr/bin/env bash
# Демо-сценарий для лабораторной работы №5.
# Создаёт несколько кампаний разных акций, отписывает часть номеров,
# отправляет все, затем дёргает /stats и сравнивает с CLI-режимом --stats.
set -e

BASE="${BASE:-http://localhost:18080}"

echo "=== /health ==="
curl -s "$BASE/health"
echo

echo "=== создаём 3 кампании акции 'Скидка 30%' ==="
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"name":"Скидка 30%","phone":"79991111111","message_template":"Привет, {name}!","recipient_name":"Иван"}' \
  "$BASE/campaigns"; echo
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"name":"Скидка 30%","phone":"79992222222","message_template":"Привет, {name}!","recipient_name":"Маша"}' \
  "$BASE/campaigns"; echo
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"name":"Скидка 30%","phone":"79993333333","message_template":"Привет, {name}!","recipient_name":"Петя"}' \
  "$BASE/campaigns"; echo

echo "=== создаём 2 кампании акции 'Чёрная пятница' ==="
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"name":"Чёрная пятница","phone":"79994444444","message_template":"-50%!","recipient_name":"Аня"}' \
  "$BASE/campaigns"; echo
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"name":"Чёрная пятница","phone":"79995555555","message_template":"-50%!","recipient_name":"Сергей"}' \
  "$BASE/campaigns"; echo

echo "=== отписываем 79993333333 (Петя) и 79995555555 (Сергей) ==="
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"phone":"79993333333"}' "$BASE/unsubscribe"; echo
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"phone":"79995555555"}' "$BASE/unsubscribe"; echo

echo "=== отправляем все 5 кампаний ==="
for i in 1 2 3 4 5; do
  curl -s -X POST "$BASE/campaigns/${i}/send"; echo
done

echo "=== GET /stats (запрос + сводка в терминал сервера) ==="
curl -s "$BASE/stats"; echo
