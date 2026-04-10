# Lower Frontier Ladder Scope Memo 90

## purpose

이 메모는 `support8 antecedent15 shell theorem` 아래의 lower frontier ladder를 코드의 natural theorem-data object granularity로 드러내기 위한 범위 메모다.

## direct dependency subset for support8 antecedent15 shell theorem

현재 코드 기준 direct dependency subset은 다음 `19`개 item이다.

- `support_plus_one_frontier`
- `antecedent_plus_one_frontier`
- `mixed_outside_bounded_frontier`
- `antecedent_plus_two_frontier`
- `support8_antecedent5_frontier`
- `antecedent_plus_three_frontier`
- `support8_antecedent6_frontier`
- `antecedent_plus_four_frontier`
- `support8_antecedent7_frontier`
- `antecedent_plus_five_frontier`
- `support8_antecedent8_frontier`
- `antecedent_plus_six_frontier`
- `support8_antecedent9_frontier`
- `antecedent_plus_seven_frontier`
- `support8_antecedent10_frontier`
- `antecedent_plus_ten_frontier`
- `support8_antecedent13_frontier`
- `antecedent_plus_eleven_frontier`
- `support8_antecedent14_frontier`

이 subset은 `lower_frontier_direct_dependency_item_keys_()`가 authoritative source다.

## first-class inventory extension

current shell-theorem direct dependency는 아니지만 first-class inventory에는 아래 `4`개도 같이 노출한다.

- `antecedent_plus_eight_frontier`
- `support8_antecedent11_frontier`
- `antecedent_plus_nine_frontier`
- `support8_antecedent12_frontier`

이 `4`개는 `lower_frontier_inventory_item_keys_()`에서 inventory-only row로 유지된다.

## human scope mapping

- `67`: `support_plus_one_frontier`, `antecedent_plus_one_frontier`
- `69`: `mixed_outside_bounded_frontier`
- `70`: shell5 pair
- `71`: shell6 pair
- `72`: shell7 pair, shell8 pair
- `74`: shell9 pair
- `75`: shell10 pair
- `76`: shell11 pair
- `77`: shell12 pair, shell13 pair
- `79`: shell14 pair

## direct consumer summary

- shell13 direct base: `support_plus_one_frontier`, `antecedent_plus_one_frontier`, `mixed_outside_bounded_frontier`, shell5 pair, shell6 pair, shell7 pair, shell8 pair, shell9 pair, shell10 pair, shell13 pair
- shell14 extension: shell13 theorem plus shell14 pair
- shell15 extension: shell14 theorem plus shell15 frontier pair
- inventory-only rows: shell11 pair, shell12 pair는 current shell15 theorem direct dependency subset 밖에 있다

## consequence

이번 라운드에서 mixed root item을 줄이려면 lower ladder 전체 `67..79`를 한 번에 freshize할 필요는 없다.  
코드 기준 최소 direct subset은 위 `19`개이며, shell11/shell12 pair는 first-class inventory에는 남기되 shell15 theorem freshization의 direct blocker로 취급하면 안 된다.
