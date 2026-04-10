# Lower Frontier Ladder Feasibility 90

## target

이번 라운드의 immediate target은 `support8 antecedent15 shell theorem`을 `mixed`에서 `fresh_current_runtime_generated`로 올리는 것이다.

## grounded feasibility answer

### 1. lower ladder 전체를 freshize해야 하는가

아니다.  
코드 기준 minimal direct subset은 `lower_frontier_direct_dependency_item_keys_()`에 있는 `19`개다.

### 2. 일부 high ladder subset만으로 충분한가

충분하지 않다.  
shell15 theorem은 shell14 theorem을 통해 shell13 base subset 전체를 닫아야 하므로, 최소 current-constructor set은 아래를 포함해야 한다.

- `support_plus_one_frontier`
- `antecedent_plus_one_frontier`
- `mixed_outside_bounded_frontier`
- shell5 pair
- shell6 pair
- shell7 pair
- shell8 pair
- shell9 pair
- shell10 pair
- shell13 pair
- shell14 pair

### 3. 어떤 subset은 보고용 imported metadata인가

현재 코드 기준 shell11 pair와 shell12 pair는 shell15 theorem direct dependency subset 밖에 있다.  
따라서 이번 라운드에서는 first-class inventory row로는 유지하되, shell theorem freshization의 필수 blocker로 취급하지 않는 것이 truthful하다.

## constructor strategy

현실적인 current constructor 전략은 다음과 같다.

- pass1:
  - lower-ladder direct dependency subset을 fresh scan으로 materialize
  - candidate universe / local exact / plus-one cache를 current runtime artifact로 쓴다
- pass2/pass3:
  - 같은 current source fingerprint에서 생성된 runtime audit와 cache artifact를 authoritative source로 다시 읽는다
  - imported closed output은 equality oracle / compatibility fallback으로만 남긴다

## promotion rule

`support8 antecedent15 shell theorem`은 아래가 모두 성립할 때만 fresh promotion 후보가 된다.

- direct lower-ladder subset `19`개가 current runtime에서 current-authoritative constructor path를 통해 materialize됨
- consumer-visible counts가 legacy theorem object와 일치함
- required runtime artifacts가 nonempty임
- pass2/pass3 cache-assisted rerun에서 same source fingerprint 기준으로 안정적으로 재현됨

## likely downstream effect

위 조건이 충족되면:

- `support8 antecedent15 shell theorem`은 fresh promotion 후보가 된다
- `support8 tail obstruction chain theorem`은 shell theorem + tail pattern + audits만 의존하므로 fresh로 재평가될 가능성이 높다
- `support8 authoritative completion lock`도 shell theorem / tail chain / shell15 frontier / audits가 모두 fresh이면 fresh로 재평가될 가능성이 높다

## remaining caveat

shell11/shell12 pair는 이번 라운드의 minimal direct blocker는 아니지만, lower-ladder inventory에는 남아 있으므로 future provenance 설명에서는 `direct dependency subset outside current shell15 theorem path`라는 caveat를 명시해야 한다.
