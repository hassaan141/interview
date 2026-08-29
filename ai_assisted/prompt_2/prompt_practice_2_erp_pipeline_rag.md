# Practice Interview Prompt 2: ERP Data Pipeline + Manufacturing Knowledge Agent

## Context

A new manufacturing customer is onboarding onto Dryft. They have an SAP ERP with messy, inconsistent data — part descriptions in mixed languages, duplicate suppliers, and no standardized BOMs (Bills of Materials). You need to build the ingestion pipeline and a knowledge agent that can answer natural language questions about their operations.

## Part A — Dagster Pipeline (10 min)

Design a Dagster pipeline (using assets) that:

1. **`raw_erp_parts`** — Pulls parts master data from SAP via REST API. Handle pagination, rate limits, and partial failures (some records may be malformed).
2. **`cleaned_parts`** — Normalizes descriptions (lowercase, strip HTML, translate non-English to English), deduplicates by fuzzy-matching on name + supplier, validates with Pydantic.
3. **`embedded_parts`** — Generates embeddings for each part description using an embedding model, stores into Postgres with pgvector.

For each asset, describe:
- Input/output types
- How you handle failures and retries
- What metadata you'd log

## Part B — FastAPI Integration (10 min)

Build endpoints that support the pipeline and the downstream agent:

1. `POST /erp-sync/{customer_id}/trigger` — Kicks off the Dagster pipeline for a customer. Returns a job run ID. How do you handle long-running jobs in a request/response cycle?
2. `GET /erp-sync/{customer_id}/status/{run_id}` — Returns pipeline status (running/success/failed) with per-asset details.
3. `POST /knowledge/ask` — Takes a natural language question + customer_id, retrieves relevant parts/supplier context via pgvector, and returns an answer with sources.

Think about:
- Async patterns (background tasks vs. polling vs. webhooks)
- Multi-tenancy — how do you isolate customer data?
- Rate limiting the `/ask` endpoint (LLM calls are expensive)

## Part C — RAG Agent (15 min)

Build a knowledge agent using Pydantic AI that answers questions like:
- "Which suppliers can deliver aluminum housings within 2 weeks?"
- "What's our average lead time for electronic components?"
- "Find me alternatives to part X that are cheaper"

Design:
- The retrieval strategy: what do you embed, how do you chunk, what metadata filters do you apply before vector search?
- Agent tools: `search_parts(query, filters)`, `get_supplier_details(supplier_id)`, `get_inventory_status(part_ids)`
- The structured output: answer text + source parts/suppliers + confidence
- How you'd use Langfuse to track retrieval quality (what metrics matter?)

## Part D — Discussion (10 min)

- Customer says search results are bad — "I search for 'M6 bolts' and get 'M8 flanges'." Walk through your debugging process. Is it an embedding issue, chunking issue, or data quality issue? How do you tell?
- How would you handle a customer whose ERP has 500k parts? What changes in your pipeline and retrieval strategy?
- The agent hallucinates a supplier that doesn't exist. How do you prevent this architecturally (not just prompt-level)?
- How do you handle schema changes when the ERP updates their API?
