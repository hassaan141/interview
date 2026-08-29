# Procurement Agent — Plan & Spec

**Simple understanding of the problem**
- Build an AI-assisted procurement agent that helps users gather supplier information, evaluate quotes, manage purchase requests, and recommend next actions. The agent should be able to interpret user intents, ask clarifying questions, query structured data (catalogs/quotes), and produce actionable outputs (recommendations, draft POs, summary reports).

**Explicit requirements (from the prompt / inferred direct asks)**n- Provide a working plan and a single document describing problem, requirements, architecture, implementation, and basic testing.
- The agent must handle procurement-specific tasks: supplier lookup, RFQ evaluation, comparison, and recommendations.
- Produce a minimal, testable prototype implementation (scripts or API) later as next step.

**Implicit requirements and assumptions**
- Privacy: sensitive procurement data should be handled carefully; prototype can use synthetic or redacted sample data.
- There will be structured data sources (CSV/JSON of suppliers, quotes, catalogs) and optionally unstructured inputs (emails, free-text requests).
- Usability: the agent should ask clarifying questions when inputs are ambiguous.
- Extensibility: design should allow swapping LLM provider, adding connectors (ERP, email), and expanding prompt templates.

**Constraints**
- Keep the initial implementation minimal and runnable locally.
- Prefer widely-available tools and languages (Python + small libs) for the prototype.
- No production-grade security or deployment required in this phase.

**High-level architecture**
- User Interface: CLI or minimal script that accepts questions/requests and returns agent responses.
- Prompt Manager: holds prompt templates, retrieval augmentation logic, and temperature/LLM settings.
- Data Connectors: simple adapters to read supplier/quote data from CSV/JSON or local files.
- Reasoning Layer (LLM): calls an LLM via a provider wrapper (abstracted) to generate responses, evaluate options, and draft outputs.
- Orchestrator / Controller: handles dialogue state, clarifying Qs, calls to data connectors, and invokes the LLM with prepared context.
- Tests: unit tests for prompt generation, data loading, and simple end-to-end sample scenarios.

Component diagram (conceptual):
User CLI <-> Orchestrator <-> {Prompt Manager, Data Connectors, LLM Provider}

**Proposed implementation**
- Language: Python 3.10+
- Libraries: requests (if external LLM), pydantic (data models), pytest (tests), pandas (optional for CSV handling), dotenv (config).
- Directory layout (suggested):
  - `procurement_agent/`
    - `cli.py` — minimal CLI to submit requests and show replies
    - `orchestrator.py` — dialogue + workflow orchestration
    - `prompts.py` — prompt templates and helper functions
    - `connectors/` — `csv_connector.py`, `sample_data.json`
    - `llm/` — `provider.py` (abstract), `mock_provider.py` (for local tests)
    - `models.py` — `Supplier`, `Quote`, `Request`, `Recommendation` (pydantic)
    - `tests/` — pytest tests for components
- LLM abstraction: implement a `LLMProvider` interface with `generate(prompt, temperature, max_tokens)`; include `MockProvider` that returns deterministic responses for tests.

Prompt design (examples)
- Intent classification prompt (short):
  "Given the user input, classify intent among: [supplier_lookup, rfq_evaluate, create_po, clarify]. Return JSON: {intent: ..., confidence: ..., slots: {...}}"

- RFQ evaluation prompt (template):
  Provide the LLM: supplier attributes, quote line items, delivery terms, and evaluation criteria (price, lead time, warranty). Ask for a scored recommendation and short rationale.

Example minimal prompt (RFQ):
"You are a procurement assistant. Evaluate the following quotes against criteria (price, delivery time, reliability). Data: <insert JSON>. Provide JSON response: {winner_supplier_id, scores: {price: X, delivery: Y, reliability: Z}, rationale: '...'}"

Data model (core fields)
- Supplier: id, name, rating, lead_time_days, contact
- Quote: id, supplier_id, total_price, currency, lead_time_days, items[]
- Request: id, requester, items[], due_date, priority
- Recommendation: chosen_supplier_id, reason, actions[]

Sample flows
1. Supplier lookup: user: "Find suppliers for 10mm bolts" -> Orchestrator queries catalog, returns top matches, asks clarifying Q if none found.
2. RFQ evaluation: user: "Compare quotes for part X" -> Load quotes JSON, build prompt with criteria, ask LLM to evaluate, return winner and rationale.
3. Create PO draft: user: "Create PO for quote #Q123" -> Generate draft PO text and confirm details with user.

**Basic testing plan**
- Unit tests
  - Data connectors: load CSV/JSON -> assert parsed models match expected values.
  - Prompt templates: ensure templates fill correctly and required fields present.
  - LLM mock integration: feed `MockProvider` and assert orchestrator parses expected JSON outputs.
- Integration tests (end-to-end using MockProvider)
  - Scenario: RFQ evaluation using sample quotes -> assert recommended supplier matches expected.
  - Scenario: Supplier lookup returns N items for given keyword.
- Manual tests
  - Run `python -m procurement_agent.cli` and try sample queries: supplier lookup, RFQ evaluate, create PO draft.

Example pytest test (concept)
- test_rfq_evaluation_with_mock
  - load sample quotes (3 suppliers)
  - call orchestrator.evaluate_rfq(request)
  - assert that response['winner_supplier_id'] == 'S2' (expected mock result)

**Acceptance criteria**
- A single spec doc exists describing problem, requirements, architecture, implementation approach, and basic tests.
- A minimal prototype (next sprint) can be run locally using `MockProvider` and sample data.
- Tests cover data loading, prompt templates, and one end-to-end RFQ evaluation scenario using mock LLM.

**Next steps (implementation roadmap)**
1. Implement repository skeleton and `MockProvider` with sample data.
2. Implement `models.py` and `connectors/csv_connector.py`.
3. Implement `prompts.py` and simple `orchestrator.py` for two flows: supplier lookup and RFQ evaluate.
4. Add `cli.py` and basic pytest tests.
5. Replace `MockProvider` with a real LLM provider (env-configured) and run integration tests.

**Resources & notes**
- Use synthetic sample data to avoid leaking sensitive procurement information.
- Keep prompts strict about returning JSON to simplify parsing.

---

Created by: GitHub Copilot
Date: 2026-06-04
